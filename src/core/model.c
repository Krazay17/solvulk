#include "sol_core.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

// Apply a 4x4 world matrix to a position
static void TransformPos(const float m[16], const float in[3], float out[3])
{
    out[0] = m[0]*in[0] + m[4]*in[1] + m[8] *in[2] + m[12];
    out[1] = m[1]*in[0] + m[5]*in[1] + m[9] *in[2] + m[13];
    out[2] = m[2]*in[0] + m[6]*in[1] + m[10]*in[2] + m[14];
}

// Apply only the rotation/scale part of a 4x4 matrix to a normal
// (no translation; for non-uniform scale you'd need the inverse-transpose,
// but for uniform scale + rotation this is fine)
static void TransformNrm(const float m[16], const float in[3], float out[3])
{
    out[0] = m[0]*in[0] + m[4]*in[1] + m[8] *in[2];
    out[1] = m[1]*in[0] + m[5]*in[1] + m[9] *in[2];
    out[2] = m[2]*in[0] + m[6]*in[1] + m[10]*in[2];
    
    // Renormalize (handles uniform scale)
    float len = sqrtf(out[0]*out[0] + out[1]*out[1] + out[2]*out[2]);
    if (len > 1e-8f) {
        out[0] /= len;
        out[1] /= len;
        out[2] /= len;
    }
}

// Count meshes by walking the scene's node hierarchy.
// We count one SolMesh per (node, primitive) pair that has a mesh attached.
static void CountNodeMeshes(cgltf_node *node, uint32_t *outMeshCount,
                            uint32_t *outVertexCount, uint32_t *outIndexCount)
{
    if (node->mesh) {
        for (cgltf_size p = 0; p < node->mesh->primitives_count; p++) {
            cgltf_primitive *prim = &node->mesh->primitives[p];
            
            cgltf_accessor *posAcc = NULL;
            for (cgltf_size a = 0; a < prim->attributes_count; a++)
                if (prim->attributes[a].type == cgltf_attribute_type_position)
                    posAcc = prim->attributes[a].data;
            
            if (!posAcc) continue;
            
            (*outMeshCount)++;
            (*outVertexCount) += (uint32_t)posAcc->count;
            (*outIndexCount) += prim->indices ? (uint32_t)prim->indices->count : 0;
        }
    }
    
    for (cgltf_size c = 0; c < node->children_count; c++)
        CountNodeMeshes(node->children[c], outMeshCount, outVertexCount, outIndexCount);
}

// Recursively process a node and its children, baking the world transform
// into each vertex.
static void ProcessNode(cgltf_node *node, SolModel *model,
                        uint32_t *meshIdx, uint32_t *vOff, uint32_t *iOff)
{
    if (node->mesh) {
        // Get this node's full world transform (walks up parents internally)
        float worldMat[16];
        cgltf_node_transform_world(node, worldMat);
        
        for (cgltf_size p = 0; p < node->mesh->primitives_count; p++) {
            cgltf_primitive *prim = &node->mesh->primitives[p];
            
            cgltf_accessor *posAcc = NULL, *nrmAcc = NULL, *uvAcc = NULL;
            for (cgltf_size a = 0; a < prim->attributes_count; a++) {
                cgltf_attribute *attr = &prim->attributes[a];
                if (attr->type == cgltf_attribute_type_position) posAcc = attr->data;
                if (attr->type == cgltf_attribute_type_normal)   nrmAcc = attr->data;
                if (attr->type == cgltf_attribute_type_texcoord) uvAcc  = attr->data;
            }
            if (!posAcc) continue;
            
            SolMesh *dst = &model->meshes[*meshIdx];
            dst->vertexCount  = (uint32_t)posAcc->count;
            dst->indexCount   = prim->indices ? (uint32_t)prim->indices->count : 0;
            dst->vertexOffset = *vOff;
            dst->indexOffset  = *iOff;
            
            // Material
            dst->material = (SolMaterial){.baseColor = {1, 1, 1, 1}, .roughness = 1.0f};
            if (prim->material && prim->material->has_pbr_metallic_roughness) {
                cgltf_pbr_metallic_roughness *pbr = &prim->material->pbr_metallic_roughness;
                memcpy(dst->material.baseColor, pbr->base_color_factor, 16);
                dst->material.metallic  = pbr->metallic_factor;
                dst->material.roughness = pbr->roughness_factor;
            }
            
            // Vertices — read local, transform to world, store
            for (uint32_t v = 0; v < dst->vertexCount; v++) {
                SolVertex *vert = &model->vertices[*vOff + v];
                
                float localPos[3], localNrm[3] = {0, 0, 0};
                cgltf_accessor_read_float(posAcc, v, localPos, 3);
                if (nrmAcc) cgltf_accessor_read_float(nrmAcc, v, localNrm, 3);
                if (uvAcc)  cgltf_accessor_read_float(uvAcc,  v, vert->uv, 2);
                
                TransformPos(worldMat, localPos, vert->position);
                if (nrmAcc) TransformNrm(worldMat, localNrm, vert->normal);
            }
            
            // Indices
            if (prim->indices)
                for (uint32_t i = 0; i < dst->indexCount; i++)
                    model->indices[*iOff + i] = (uint32_t)cgltf_accessor_read_index(prim->indices, i);
            
            *vOff += dst->vertexCount;
            *iOff += dst->indexCount;
            (*meshIdx)++;
        }
    }
    
    // Recurse into children
    for (cgltf_size c = 0; c < node->children_count; c++)
        ProcessNode(node->children[c], model, meshIdx, vOff, iOff);
}

SolModel Parse_Model(SolResource res)
{
    SolModel model = {0};
    
    cgltf_options options = {0};
    cgltf_data   *data    = NULL;
    if (cgltf_parse(&options, res.data, res.size, &data) != cgltf_result_success)
        return model;
    cgltf_load_buffers(&options, data, NULL);
    
    // Pick the default scene (or first scene if no default)
    cgltf_scene *scene = data->scene ? data->scene : &data->scenes[0];
    if (!scene) {
        cgltf_free(data);
        return model;
    }
    
    // Pass 1: count meshes and totals by walking the scene hierarchy
    for (cgltf_size n = 0; n < scene->nodes_count; n++)
        CountNodeMeshes(scene->nodes[n], &model.mesh_count, 
                        &model.vertex_count, &model.indice_count);
    
    if (model.mesh_count == 0) {
        cgltf_free(data);
        return model;
    }
    
    // Allocate
    model.meshes   = calloc(model.mesh_count, sizeof(SolMesh));
    model.vertices = calloc(model.vertex_count, sizeof(SolVertex));
    model.indices  = malloc(model.indice_count * sizeof(uint32_t));
    
    // Pass 2: fill, applying world transforms
    uint32_t meshIdx = 0, vOff = 0, iOff = 0;
    for (cgltf_size n = 0; n < scene->nodes_count; n++)
        ProcessNode(scene->nodes[n], &model, &meshIdx, &vOff, &iOff);
    
    // Build tris from final world-space vertices
    model.tri_count = model.indice_count / 3;
    model.tris      = malloc(model.tri_count * sizeof(SolTri));
    
    uint32_t triIdx = 0;
    for (uint32_t m = 0; m < model.mesh_count; m++) {
        SolMesh *mesh = &model.meshes[m];
        for (uint32_t i = 0; i < mesh->indexCount; i += 3) {
            SolTri *t = &model.tris[triIdx++];
            
            uint32_t i0 = model.indices[mesh->indexOffset + i + 0];
            uint32_t i1 = model.indices[mesh->indexOffset + i + 1];
            uint32_t i2 = model.indices[mesh->indexOffset + i + 2];
            
            t->a = *(vec3s *)model.vertices[mesh->vertexOffset + i0].position;
            t->b = *(vec3s *)model.vertices[mesh->vertexOffset + i1].position;
            t->c = *(vec3s *)model.vertices[mesh->vertexOffset + i2].position;
            
            vec3s e1    = glms_vec3_sub(t->b, t->a);
            vec3s e2    = glms_vec3_sub(t->c, t->a);
            vec3s cross = glms_vec3_cross(e1, e2);
            float len   = glms_vec3_norm(cross);
            t->normal   = len > 0.00001f ? glms_vec3_scale(cross, 1.0f / len) : (vec3s){{0, 1, 0}};
            
            t->center = glms_vec3_scale(glms_vec3_add(glms_vec3_add(t->a, t->b), t->c), 1.0f / 3.0f);
            
            float da  = glms_vec3_norm(glms_vec3_sub(t->a, t->center));
            float db  = glms_vec3_norm(glms_vec3_sub(t->b, t->center));
            float dc  = glms_vec3_norm(glms_vec3_sub(t->c, t->center));
            t->bounds = fmaxf(da, fmaxf(db, dc));
        }
    }
    
    cgltf_free(data);
    return model;
}

void Sol_FreeModel(SolModel *model)
{
    free(model->vertices);
    free(model->indices);
    free(model->meshes);
    free(model->tris);
    memset(model, 0, sizeof(SolModel));
}