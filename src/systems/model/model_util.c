#include "model.h"
#include "sol_core.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

static SolSkeleton ParseSkeleton(cgltf_data *data);

static void CountNodeMeshes(cgltf_node *node, uint32_t *outMeshCount, uint32_t *outVertexCount,
                            uint32_t *outIndexCount);
static void ProcessNode(cgltf_node *node, SolModel *model, uint32_t *meshIdx, uint32_t *vOff, uint32_t *iOff);
static void Sample_Channel(SolAnimChannel *ch, float t, float *out);

void Sol_FreeModel(SolModel *model)
{
    free(model->vertices);
    free(model->indices);
    free(model->meshes);
    free(model->tris);
    memset(model, 0, sizeof(SolModel));
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
    if (!scene)
    {
        cgltf_free(data);
        return model;
    }

    // Pass 1: count meshes and totals by walking the scene hierarchy
    for (cgltf_size n = 0; n < scene->nodes_count; n++)
        CountNodeMeshes(scene->nodes[n], &model.mesh_count, &model.vertex_count, &model.indice_count);

    if (model.mesh_count == 0)
    {
        cgltf_free(data);
        return model;
    }

    model.skeleton = ParseSkeleton(data);

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

static SolSkeleton ParseSkeleton(cgltf_data *data)
{
    SolSkeleton skel = {0};
    if (data->skins_count == 0)
        return skel;

    cgltf_skin *skin = &data->skins[0]; // assume first skin
    skel.boneCount   = (int)skin->joints_count;
    skel.bones       = calloc(skel.boneCount, sizeof(SolBone));

    // Build a node-pointer → bone-index map (only for joints in this skin)
    // For each joint, find its parent's index in the joints array.
    for (int i = 0; i < skel.boneCount; i++)
    {
        SolBone    *bone = &skel.bones[i];
        cgltf_node *node = skin->joints[i];

        if (node->name)
            strncpy(bone->name, node->name, 63);

        // Find parent bone index
        bone->parent           = -1;
        cgltf_node *parentNode = node->parent;
        if (parentNode)
        {
            for (int j = 0; j < skel.boneCount; j++)
            {
                if (skin->joints[j] == parentNode)
                {
                    bone->parent = j;
                    break;
                }
            }
        }

        // Read inverse bind matrix
        if (skin->inverse_bind_matrices)
        {
            float m[16];
            cgltf_accessor_read_float(skin->inverse_bind_matrices, i, m, 16);
            memcpy(bone->inverseBind, m, sizeof(mat4));
        }
        else
        {
            glm_mat4_identity(bone->inverseBind);
        }

        // Read rest TRS from the node
        if (node->has_translation)
            memcpy(&bone->restTrans, node->translation, sizeof(vec3));
        if (node->has_rotation)
        {
            // glTF stores quaternions as (x, y, z, w); cglm uses the same
            memcpy(&bone->restRot, node->rotation, sizeof(vec4));
        }
        else
        {
            bone->restRot = (versors){{0, 0, 0, 1}};
        }
        if (node->has_scale)
            memcpy(&bone->restScale, node->scale, sizeof(vec3));
        else
            bone->restScale = (vec3s){{1, 1, 1}};
    }

    // Parse animations
    skel.animationCount = (int)data->animations_count;
    if (skel.animationCount > 0)
    {
        skel.animations = calloc(skel.animationCount, sizeof(SolAnimation));

        for (int a = 0; a < skel.animationCount; a++)
        {
            cgltf_animation *src  = &data->animations[a];
            SolAnimation    *anim = &skel.animations[a];

            if (src->name)
                strncpy(anim->name, src->name, 63);

            anim->channelCount = (int)src->channels_count;
            anim->channels     = calloc(anim->channelCount, sizeof(SolAnimChannel));

            float maxTime = 0;
            for (int c = 0; c < anim->channelCount; c++)
            {
                cgltf_animation_channel *srcCh = &src->channels[c];
                SolAnimChannel          *dstCh = &anim->channels[c];

                // Map target node → bone index
                dstCh->boneIndex = -1;
                for (int j = 0; j < skel.boneCount; j++)
                {
                    if (skin->joints[j] == srcCh->target_node)
                    {
                        dstCh->boneIndex = j;
                        break;
                    }
                }
                if (dstCh->boneIndex < 0)
                    continue;

                // Path
                switch (srcCh->target_path)
                {
                case cgltf_animation_path_type_translation:
                    dstCh->path = ANIM_PATH_TRANSLATION;
                    break;
                case cgltf_animation_path_type_rotation:
                    dstCh->path = ANIM_PATH_ROTATION;
                    break;
                case cgltf_animation_path_type_scale:
                    dstCh->path = ANIM_PATH_SCALE;
                    break;
                default:
                    continue;
                }

                // Keyframe data
                cgltf_animation_sampler *sampler  = srcCh->sampler;
                int                      keyCount = (int)sampler->input->count;
                dstCh->keyCount                   = keyCount;

                dstCh->times = malloc(keyCount * sizeof(float));
                for (int k = 0; k < keyCount; k++)
                    cgltf_accessor_read_float(sampler->input, k, &dstCh->times[k], 1);

                int valuesPerKey = (dstCh->path == ANIM_PATH_ROTATION) ? 4 : 3;
                dstCh->values    = malloc(keyCount * valuesPerKey * sizeof(float));
                for (int k = 0; k < keyCount; k++)
                    cgltf_accessor_read_float(sampler->output, k, &dstCh->values[k * valuesPerKey], valuesPerKey);

                if (keyCount > 0 && dstCh->times[keyCount - 1] > maxTime)
                    maxTime = dstCh->times[keyCount - 1];
            }

            anim->duration = maxTime;
        }
    }

    return skel;
}

// Count meshes by walking the scene's node hierarchy.
// We count one SolMesh per (node, primitive) pair that has a mesh attached.
static void CountNodeMeshes(cgltf_node *node, uint32_t *outMeshCount, uint32_t *outVertexCount, uint32_t *outIndexCount)
{
    if (node->mesh)
    {
        for (cgltf_size p = 0; p < node->mesh->primitives_count; p++)
        {
            cgltf_primitive *prim = &node->mesh->primitives[p];

            cgltf_accessor *posAcc = NULL;
            for (cgltf_size a = 0; a < prim->attributes_count; a++)
                if (prim->attributes[a].type == cgltf_attribute_type_position)
                    posAcc = prim->attributes[a].data;

            if (!posAcc)
                continue;

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
static void ProcessNode(cgltf_node *node, SolModel *model, uint32_t *meshIdx, uint32_t *vOff, uint32_t *iOff)
{
    if (node->mesh)
    {
        // Get this node's full world transform (walks up parents internally)
        float worldMat[16];
        cgltf_node_transform_world(node, worldMat);

        for (cgltf_size p = 0; p < node->mesh->primitives_count; p++)
        {
            cgltf_primitive *prim = &node->mesh->primitives[p];

            cgltf_accessor *posAcc = NULL, *nrmAcc = NULL, *uvAcc = NULL;
            for (cgltf_size a = 0; a < prim->attributes_count; a++)
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

            SolMesh *dst      = &model->meshes[*meshIdx];
            dst->vertexCount  = (uint32_t)posAcc->count;
            dst->indexCount   = prim->indices ? (uint32_t)prim->indices->count : 0;
            dst->vertexOffset = *vOff;
            dst->indexOffset  = *iOff;

            // Bones
            cgltf_accessor *jntAcc = NULL, *wghAcc = NULL;
            for (cgltf_size a = 0; a < prim->attributes_count; a++)
            {
                cgltf_attribute *attr = &prim->attributes[a];
                if (attr->type == cgltf_attribute_type_joints)
                    jntAcc = attr->data;
                if (attr->type == cgltf_attribute_type_weights)
                    wghAcc = attr->data;
            }

            // Material
            dst->material = (SolMaterial){.baseColor = {1, 1, 1, 1}, .roughness = 1.0f};
            if (prim->material && prim->material->has_pbr_metallic_roughness)
            {
                cgltf_pbr_metallic_roughness *pbr = &prim->material->pbr_metallic_roughness;
                memcpy(dst->material.baseColor, pbr->base_color_factor, 16);
                dst->material.metallic  = pbr->metallic_factor;
                dst->material.roughness = pbr->roughness_factor;
            }

            // Vertices — read local, transform to world, store
            for (uint32_t v = 0; v < dst->vertexCount; v++)
            {
                SolVertex *vert = &model->vertices[*vOff + v];

                float localPos[3], localNrm[3] = {0, 0, 0};
                cgltf_accessor_read_float(posAcc, v, localPos, 3);
                if (nrmAcc)
                    cgltf_accessor_read_float(nrmAcc, v, localNrm, 3);
                if (uvAcc)
                    cgltf_accessor_read_float(uvAcc, v, vert->uv, 2);

                TransformPos(worldMat, localPos, vert->position);
                if (nrmAcc)
                    TransformNrm(worldMat, localNrm, vert->normal);

                // Joints stored as uint8/uint16 indices — cgltf reads as floats here for simplicity
                if (jntAcc)
                {
                    float jf[4];
                    cgltf_accessor_read_float(jntAcc, v, jf, 4);
                    vert->boneIndices[0] = (uint32_t)jf[0];
                    vert->boneIndices[1] = (uint32_t)jf[1];
                    vert->boneIndices[2] = (uint32_t)jf[2];
                    vert->boneIndices[3] = (uint32_t)jf[3];
                }
                if (wghAcc)
                    cgltf_accessor_read_float(wghAcc, v, vert->boneWeights, 4);
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

// Sample one channel at a given time, returning interpolated value
static void Sample_Channel(SolAnimChannel *ch, float t, float *out)
{
    if (ch->keyCount == 0)
        return;
    if (ch->keyCount == 1 || t <= ch->times[0])
    {
        int valuesPerKey = (ch->path == ANIM_PATH_ROTATION) ? 4 : 3;
        memcpy(out, ch->values, valuesPerKey * sizeof(float));
        return;
    }
    if (t >= ch->times[ch->keyCount - 1])
    {
        int valuesPerKey = (ch->path == ANIM_PATH_ROTATION) ? 4 : 3;
        memcpy(out, &ch->values[(ch->keyCount - 1) * valuesPerKey], valuesPerKey * sizeof(float));
        return;
    }

    // Find bracketing keyframes
    int k1 = 1;
    while (k1 < ch->keyCount && ch->times[k1] < t)
        k1++;
    int k0 = k1 - 1;

    float t0 = ch->times[k0];
    float t1 = ch->times[k1];
    float a  = (t - t0) / (t1 - t0);

    int    valuesPerKey = (ch->path == ANIM_PATH_ROTATION) ? 4 : 3;
    float *v0           = &ch->values[k0 * valuesPerKey];
    float *v1           = &ch->values[k1 * valuesPerKey];

    if (ch->path == ANIM_PATH_ROTATION)
    {
        glm_quat_slerp(v0, v1, a, out);
    }
    else
    {
        for (int i = 0; i < 3; i++)
            out[i] = v0[i] + a * (v1[i] - v0[i]);
    }
}

// Compute skinning matrices for one entity at one moment in time
void Sol_Skeleton_Pose(SolSkeleton *skel, int animIndex, float time, mat4 *outSkinMatrices)
{
    // 1. Start each bone with its rest pose
    static vec3s   localT[MAX_BONES];
    static versors localR[MAX_BONES];
    static vec3s   localS[MAX_BONES];
    static mat4    world[MAX_BONES];

    for (int i = 0; i < skel->boneCount; i++)
    {
        localT[i] = skel->bones[i].restTrans;
        localR[i] = skel->bones[i].restRot;
        localS[i] = skel->bones[i].restScale;
    }

    // 2. Sample animation channels and override the rest pose
    if (animIndex >= 0 && animIndex < skel->animationCount)
    {
        SolAnimation *anim = &skel->animations[animIndex];
        float         t    = fmodf(time, anim->duration); // loop the animation

        for (int c = 0; c < anim->channelCount; c++)
        {
            SolAnimChannel *ch = &anim->channels[c];
            if (ch->boneIndex < 0)
                continue;

            float sampled[4];
            Sample_Channel(ch, t, sampled);

            switch (ch->path)
            {
            case ANIM_PATH_TRANSLATION:
                memcpy(&localT[ch->boneIndex], sampled, sizeof(vec3));
                break;
            case ANIM_PATH_ROTATION:
                memcpy(&localR[ch->boneIndex], sampled, sizeof(vec4));
                break;
            case ANIM_PATH_SCALE:
                memcpy(&localS[ch->boneIndex], sampled, sizeof(vec3));
                break;
            }
        }
    }

    // 3. Build local matrices and accumulate world-space matrices via parent chain
    for (int i = 0; i < skel->boneCount; i++)
    {
        mat4 local;
        glm_mat4_identity(local);

        // T * R * S
        mat4 tMat, rMat, sMat;
        glm_translate_make(tMat, (float *)&localT[i]);
        glm_quat_mat4(localR[i].raw, rMat);
        glm_scale_make(sMat, (float *)&localS[i]);
        glm_mat4_mul(tMat, rMat, local);
        glm_mat4_mul(local, sMat, local);

        int parent = skel->bones[i].parent;
        if (parent < 0)
        {
            memcpy(world[i], local, sizeof(mat4));
        }
        else
        {
            glm_mat4_mul(world[parent], local, world[i]);
        }
    }

    // 4. Final skinning matrix = world * inverseBind
    for (int i = 0; i < skel->boneCount; i++)
    {
        glm_mat4_mul(world[i], skel->bones[i].inverseBind, outSkinMatrices[i]);
    }
}
