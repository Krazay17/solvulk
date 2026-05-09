#include "sol_core.h"

#include "model/model.h"
#include "physx.h"

void Physx_ParseModel(World *world, int id, PhysxGroup *group)
{
    CompXform *xform    = &world->xforms[id];
    SolModel  *model    = Sol_GetModel(Sol_Model_GetModelId(world, id));
    u32        oldCount = group->triCount;
    u32        newCount = oldCount + model->tri_count;

    if (newCount > group->capacity)
    {
        group->capacity = newCount * 2;
        group->tris     = realloc(group->tris, sizeof(SolTri) * group->capacity);
    }

    group->triCount = newCount;
    Transform_Tris_LocalToWorld(group->tris, id, oldCount, model, xform);

    group->ents[id].triIndexStart = oldCount;
    group->ents[id].triIndexCount = model->tri_count;
}

void Spatial_Add_Model(PhysxGroup *triGroup, int id, SolModel *model, CompXform *xform, bool hash)
{
    //SolModel *model    = Physx_ParseModel(world, id);
    u32       oldCount = triGroup->triCount;
    u32       newCount = oldCount + model->tri_count;

    triGroup->tris     = realloc(triGroup->tris, sizeof(SolTri) * newCount);
    triGroup->triCount = newCount;
    Transform_Tris_LocalToWorld(triGroup->tris, id, oldCount, model, xform);

    triGroup->ents[id].triIndexStart = oldCount;
    triGroup->ents[id].triIndexCount = model->tri_count;
    triGroup->ents[id].id            = id;
    triGroup->entCount++;

    if (hash)
    {
        SpatialTable_Clear(&triGroup->table);
        Spatial_Hash_Tris(triGroup);
    }
}

void Transform_Tris_LocalToWorld(SolTri *group, int id, int offset, SolModel *model, CompXform *xform)
{
    mat3s rot = glms_quat_mat3(xform->quat);
    for (int i = 0; i < model->tri_count; i++)
    {
        SolTri  src = model->tris[i];
        SolTri *dst = &group[offset + i];
        dst->entId  = id;

        dst->a = glms_vec3_add(glms_mat3_mulv(rot, glms_vec3_mul(src.a, xform->scale)), xform->pos);
        dst->b = glms_vec3_add(glms_mat3_mulv(rot, glms_vec3_mul(src.b, xform->scale)), xform->pos);
        dst->c = glms_vec3_add(glms_mat3_mulv(rot, glms_vec3_mul(src.c, xform->scale)), xform->pos);

        // Recompute derived data in world space
        vec3s e1    = glms_vec3_sub(dst->b, dst->a);
        vec3s e2    = glms_vec3_sub(dst->c, dst->a);
        vec3s cross = glms_vec3_cross(e1, e2);
        float len   = glms_vec3_norm(cross);
        dst->normal = len > 0.00001f ? glms_vec3_scale(cross, 1.0f / len) : (vec3s){0, 1, 0};

        dst->center = glms_vec3_scale(glms_vec3_add(glms_vec3_add(dst->a, dst->b), dst->c), 1.0f / 3.0f);

        float da    = glms_vec3_norm(glms_vec3_sub(dst->a, dst->center));
        float db    = glms_vec3_norm(glms_vec3_sub(dst->b, dst->center));
        float dc    = glms_vec3_norm(glms_vec3_sub(dst->c, dst->center));
        dst->bounds = fmaxf(da, fmaxf(db, dc));
    }
}
