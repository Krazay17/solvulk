#include "world.h"
#include "sol_core.h"
#include "sol_math.h"
#include "render/render.h"

void Model_Draw(World *world, double dt, double time)
{
    float               fdt = (float)dt;
    SparseSet_SolModel *set = Sol_Comp_Set(world, SolModel);
    for (int i = 0; i < set->cnt; i++)
    {
        int       id        = set->dense[i];
        SolModel *modelComp = &set->data[i];
        SolXform *xform     = Sol_Comp_Get(world, id, SolXform);
        ModelSSBO modelSSBO = {0};
        modelSSBO.color     = modelComp->color;

        if (Sol_Comp_Has(world, id, SolInteract))
        {
            SolInteract *interact = Sol_Comp_Get(world, id, SolInteract);
            if (interact->state & (INTERACT_HOVERED | INTERACT_DRAGGING))
                modelSSBO.flags |= (1 << 0);
        }
        if (Sol_Comp_Has(world, id, SolBuff))
        {
            if (Sol_Buff_HasBuff(world, id, BUFFKIND_INVULN))
                modelSSBO.flags |= (1 << 1);
        }

        if (Sol_Comp_Has(world, id, SolCombat))
        {
            SolCombat *combat = Sol_Comp_Get(world, id, SolCombat);
            modelSSBO.hitTime = combat->lastHitTime;
        }
        else
            modelSSBO.hitTime = -100.0f;

        if (modelComp->is2d)
        {
            modelSSBO.flags |= (1 << 2);
            float px           = UISCALE(xform->draw_pos.x + (modelComp->xOffset * xform->draw_sca.x));
            float py           = UISCALE(xform->draw_pos.y + (-modelComp->yOffset * xform->draw_sca.y));
            float pz           = xform->draw_pos.z;
            modelSSBO.position = (vec4s){px, py, pz, 1.0f};
            modelSSBO.rotation = (vec4s){xform->draw_rot.x, xform->draw_rot.y, xform->draw_rot.z, xform->draw_rot.w};
            modelSSBO.scale =
                (vec4s){UISCALE(xform->draw_sca.x), UISCALE(xform->draw_sca.y), UISCALE(xform->draw_sca.z), 1.0f};
        }
        else
        {
            modelSSBO.position =
                (vec4s){xform->draw_pos.x, xform->draw_pos.y + modelComp->yOffset, xform->draw_pos.z, 1.0f};
            modelSSBO.rotation = (vec4s){xform->draw_rot.x, xform->draw_rot.y, xform->draw_rot.z, xform->draw_rot.w};
            modelSSBO.scale    = (vec4s){xform->draw_sca.x, xform->draw_sca.y, xform->draw_sca.z, 1.0f};
        }

        if (Sol_Comp_Has(world, id, SolAnim))
        {
            SolAnim *anim = Sol_Comp_Get(world, id, SolAnim);
            Sol_Render_GetNext_Model(modelComp->modelId, &modelSSBO, &anim->pose);
        }
        else
        {
            Sol_Render_GetNext_Model(modelComp->modelId, &modelSSBO, NULL);
        }
    }
}

Xform Sol_Model_GetBoneXform(World *world, int id, const char *name)
{
    Xform        result   = {0};
    SolModel    *model    = Sol_Comp_Get(world, id, SolModel);
    SolAnim     *anim     = Sol_Comp_Get(world, id, SolAnim);
    SolXform    *xform    = Sol_Comp_Get(world, id, SolXform);
    SolSkeleton *skeleton = &loaded_models[model->modelId].skeleton;

    int boneIdx = -1;
    for (int i = 0; i < skeleton->boneCount; i++)
    {
        if (strstr(skeleton->bones[i].name, name))
        {
            boneIdx = i;
            break;
        }
    }
    if (boneIdx < 0)
    {
        result.pos  = GLMS_VEC3_ZERO;
        result.quat = GLMS_QUAT_IDENTITY;
        return result;
    }

    // 1. Get the bone's animated pose in Model Space.
    // model->bones contains: AnimatedBonePose * InverseBindPose
    // To extract the actual current Model Space position and rotation of the bone,
    // we multiply it back by the original bindPose matrix.
    mat4 bindPose;
    glm_mat4_inv(skeleton->bones[boneIdx].inverseBind, bindPose);

    mat4 boneModelSpace;
    glm_mat4_mul(anim->pose.bones[boneIdx], bindPose, boneModelSpace);

    // 2. Construct the Entity's World Transform Matrix
    mat4 entityWorld;
    glm_mat4_identity(entityWorld);

    // Apply position (including your yOffset adjustment)
    vec3 actualDrawPos = {xform->draw_pos.x, xform->draw_pos.y + (model->yOffset * xform->draw_sca.y),
                          xform->draw_pos.z};
    glm_translate(entityWorld, actualDrawPos);

    // Apply rotation
    glm_quat_rotate(entityWorld, xform->draw_rot.raw, entityWorld);

    // Apply scale
    glm_scale(entityWorld, xform->draw_sca.raw);

    // 3. Combine them: Final World Matrix = EntityWorld * BoneModelSpace
    mat4 finalWorldMat;
    glm_mat4_mul(entityWorld, boneModelSpace, finalWorldMat);

    // 4. Decompose the final matrix to extract raw position and quaternion
    // Extract Position vector directly from the 4th column
    result.pos.x = finalWorldMat[3][0];
    result.pos.y = finalWorldMat[3][1];
    result.pos.z = finalWorldMat[3][2];

    // Extract Rotation quaternion cleanly from the upper-left 3x3
    glm_mat4_quat(finalWorldMat, result.quat.raw);

    return result;
}
