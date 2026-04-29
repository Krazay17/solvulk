#include "sol_core.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"


SolModel Parse_Model(SolResource res)
{
    SolModel model = {0};

    cgltf_options options = {0};
    cgltf_data   *data    = NULL;
    if (cgltf_parse(&options, res.data, res.size, &data) != cgltf_result_success)
        return model;
    cgltf_load_buffers(&options, data, NULL);

    model.mesh_count = (uint32_t)data->meshes_count;
    model.meshes     = calloc(model.mesh_count, sizeof(SolMesh));

    // Pass 1: count totals
    for (uint32_t m = 0; m < model.mesh_count; m++)
    {
        cgltf_mesh *src = &data->meshes[m];
        if (src->primitives_count == 0)
            continue;
        cgltf_primitive *prim = &src->primitives[0];

        cgltf_accessor *posAcc = NULL;
        for (uint32_t a = 0; a < prim->attributes_count; a++)
            if (prim->attributes[a].type == cgltf_attribute_type_position)
                posAcc = prim->attributes[a].data;

        if (!posAcc)
            continue;

        model.meshes[m].vertexCount = (uint32_t)posAcc->count;
        model.meshes[m].indexCount  = prim->indices ? (uint32_t)prim->indices->count : 0;
        model.vertex_count += model.meshes[m].vertexCount;
        model.indice_count += model.meshes[m].indexCount;
    }

    // Single allocation
    model.vertices = calloc(model.vertex_count, sizeof(SolVertex));
    model.indices  = malloc(model.indice_count * sizeof(uint32_t));

    // Pass 2: fill data
    uint32_t vOff = 0, iOff = 0;
    for (uint32_t m = 0; m < model.mesh_count; m++)
    {
        cgltf_mesh *src = &data->meshes[m];
        if (src->primitives_count == 0)
            continue;
        cgltf_primitive *prim = &src->primitives[0];

        SolMesh *dst      = &model.meshes[m];
        dst->vertexOffset = vOff;
        dst->indexOffset  = iOff;

        // Material
        dst->material = (SolMaterial){.baseColor = {1, 1, 1, 1}, .roughness = 1.0f};
        if (prim->material && prim->material->has_pbr_metallic_roughness)
        {
            cgltf_pbr_metallic_roughness *pbr = &prim->material->pbr_metallic_roughness;
            memcpy(dst->material.baseColor, pbr->base_color_factor, 16);
            dst->material.metallic  = pbr->metallic_factor;
            dst->material.roughness = pbr->roughness_factor;
        }

        // Accessors
        cgltf_accessor *posAcc = NULL, *nrmAcc = NULL, *uvAcc = NULL;
        for (uint32_t a = 0; a < prim->attributes_count; a++)
        {
            cgltf_attribute *attr = &prim->attributes[a];
            if (attr->type == cgltf_attribute_type_position)
                posAcc = attr->data;
            if (attr->type == cgltf_attribute_type_normal)
                nrmAcc = attr->data;
            if (attr->type == cgltf_attribute_type_texcoord)
                uvAcc = attr->data;
        }
        if (!posAcc)
            continue;

        // Vertices
        for (uint32_t v = 0; v < dst->vertexCount; v++)
        {
            SolVertex *vert = &model.vertices[vOff + v];
            cgltf_accessor_read_float(posAcc, v, vert->position, 3);
            if (nrmAcc)
                cgltf_accessor_read_float(nrmAcc, v, vert->normal, 3);
            if (uvAcc)
                cgltf_accessor_read_float(uvAcc, v, vert->uv, 2);
        }

        // Indices
        if (prim->indices)
            for (uint32_t i = 0; i < dst->indexCount; i++)
                model.indices[iOff + i] = (uint32_t)cgltf_accessor_read_index(prim->indices, i);

        vOff += dst->vertexCount;
        iOff += dst->indexCount;
    }

    model.tri_count = model.indice_count / 3;
    model.tris      = malloc(model.tri_count * sizeof(SolTri));

    uint32_t triIdx = 0;
    for (uint32_t m = 0; m < model.mesh_count; m++)
    {
        SolMesh *mesh = &model.meshes[m];
        for (uint32_t i = 0; i < mesh->indexCount; i += 3)
        {
            SolTri *t = &model.tris[triIdx++];

            uint32_t i0 = model.indices[mesh->indexOffset + i + 0];
            uint32_t i1 = model.indices[mesh->indexOffset + i + 1];
            uint32_t i2 = model.indices[mesh->indexOffset + i + 2];

            t->a = *(vec3s *)model.vertices[mesh->vertexOffset + i0].position;
            t->b = *(vec3s *)model.vertices[mesh->vertexOffset + i1].position;
            t->c = *(vec3s *)model.vertices[mesh->vertexOffset + i2].position;

            // Face normal from edges
            vec3s e1    = glms_vec3_sub(t->b, t->a);
            vec3s e2    = glms_vec3_sub(t->c, t->a);
            vec3s cross = glms_vec3_cross(e1, e2);
            float len   = glms_vec3_norm(cross);
            t->normal   = len > 0.00001f ? glms_vec3_scale(cross, 1.0f / len) : (vec3s){0, 1, 0};

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
    memset(model, 0, sizeof(SolModel));
}