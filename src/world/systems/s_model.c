#include "world.h"
#include "sol_core.h"
#include "sol_math.h"
#include "render/render.h"

typedef struct
{
    float y_offset;
    float yaw_offset;
} ModelKindData;

const ModelKindData model_kinds[SOL_MODEL_COUNT] = {
    [MODELKIND_DUDE] =
        {
            .y_offset = -0.825f,
        },
    [MODELKIND_WIZARD] =
        {
            .y_offset = -1.5f,
        },
    [MODELKIND_ZORGON] =
        {
            .y_offset = -0.8f,
        },
    [MODELKIND_EVAN] =
        {
            .yaw_offset = GLM_PI_2f,
        },
};

void Model_Render(World *world, double dt)
{
    float fdt              = (float)dt;
    SparseSet_ScModel *set = Sol_Comp_Set(world, ScModel);
    for (int i = 0; i < set->cnt; i++)
    {
        int id              = set->dense[i];
        ScModel *model      = &set->data[i];
        ModelSSBO modelSSBO = {0};
        modelSSBO.color     = model->color;

        XformsDraw xform = Xform_GetDraw(world, id);

        if (Sol_Comp_Has(world, id, ScInteract))
        {
            ScInteract *interact = Sol_Comp_Get(world, id, ScInteract);
            if (interact->state & (INTERACT_HOVERED | INTERACT_DRAGGING))
                modelSSBO.flags |= (1 << 0);
        }
        if (Sol_Comp_Has(world, id, ScBuff))
        {
            if (Sol_Buff_HasBuff(world, id, BUFFKIND_INVULN))
                modelSSBO.flags |= (1 << 1);
        }

        if (Sol_Comp_Has(world, id, ScCombat))
        {
            ScCombat *combat  = Sol_Comp_Get(world, id, ScCombat);
            modelSSBO.hitTime = combat->lastHitTime;
        }
        else
            modelSSBO.hitTime = -100.0f;

        vec3s pos = xform.pos;
        pos.y += model_kinds[model->kind].y_offset;
        pos.y += model->yOffset;

        if (model->is2d)
        {
            modelSSBO.flags |= (1 << 2);
            float px           = UISCALE(pos.x + (model->xOffset * xform.sca.x));
            float py           = UISCALE(pos.y + (-model->yOffset * xform.sca.y));
            float pz           = pos.z;
            modelSSBO.position = (vec4s){px, py, pz, 1.0f};
            modelSSBO.rotation = (vec4s){xform.rot.x, xform.rot.y, xform.rot.z, xform.rot.w};
            modelSSBO.scale    = (vec4s){UISCALE(xform.sca.x), UISCALE(xform.sca.y), UISCALE(xform.sca.z), 1.0f};
        }
        else
        {
            modelSSBO.position = (vec4s){pos.x, pos.y, pos.z, 1.0f};
            modelSSBO.rotation = (vec4s){xform.rot.x, xform.rot.y, xform.rot.z, xform.rot.w};
            modelSSBO.scale    = (vec4s){xform.sca.x, xform.sca.y, xform.sca.z, 1.0f};
        }
        if (Sol_Comp_Has(world, id, ScAnim))
        {
            ScAnim *anim = Sol_Comp_Get(world, id, ScAnim);
            Sol_Render_GetNext_Model(model->kind, &modelSSBO, &anim->pose);
        }
        else
        {
            Sol_Render_GetNext_Model(model->kind, &modelSSBO, NULL);
        }
    }
}

void Model_Init(World *world)
{
}

SolXform Sol_Model_GetBoneXform(World *world, int id, const char *name)
{
    SolXform result       = {0};
    ScModel *model        = Sol_Comp_Get(world, id, ScModel);
    ScAnim *anim          = Sol_Comp_Get(world, id, ScAnim);
    SolSkeleton *skeleton = &loaded_models[model->kind].skeleton;
    XformsDraw xform      = Xform_GetDraw(world, id);

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
        result.pos = GLMS_VEC3_ZERO;
        result.rot = GLMS_QUAT_IDENTITY;
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
    vec3 actualDrawPos = {xform.pos.x, xform.pos.y + (model->yOffset * xform.sca.y), xform.pos.z};
    glm_translate(entityWorld, actualDrawPos);

    // Apply rotation
    glm_quat_rotate(entityWorld, xform.rot.raw, entityWorld);

    // Apply scale
    glm_scale(entityWorld, xform.sca.raw);

    // 3. Combine them: Final World Matrix = EntityWorld * BoneModelSpace
    mat4 finalWorldMat;
    glm_mat4_mul(entityWorld, boneModelSpace, finalWorldMat);

    // 4. Decompose the final matrix to extract raw position and quaternion
    // Extract Position vector directly from the 4th column
    result.pos.x = finalWorldMat[3][0];
    result.pos.y = finalWorldMat[3][1];
    result.pos.z = finalWorldMat[3][2];

    // Extract Rotation quaternion cleanly from the upper-left 3x3
    glm_mat4_quat(finalWorldMat, result.rot.raw);

    return result;
}
