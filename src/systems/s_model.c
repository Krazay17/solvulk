#include "world.h"
#include "sol_core.h"
#include "sol_math.h"
#include "sol_user.h"
#include "render/render.h"

const struct ModelKindData
{
    float y_offset;
    float yaw_offset;
} model_kinds[MODELKIND_COUNT] = {
    [MODELKIND_DUDE] =
        {
            .y_offset = -1.0f,
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
    float fdt = world->fdt;

    SparseSet_ScModel *set = Sol_Comp_Set(world, ScModel);
    for (int i = 0; i < set->cnt; i++)
    {
        int id              = set->dense[i];
        ScModel *model      = &set->data[i];
        ModelSSBO modelSSBO = {0};
        modelSSBO.color     = model->color;

        Xform xform = Xform_GetDraw(world, id);

        if (Sol_Comp_Has(world, id, ScInteract))
        {
            ScInteract *interact = Sol_Comp_Get(world, id, ScInteract);

            if (interact->is_local && (interact->state & (INTERACT_DRAGGING | INTERACT_HOVERED)))
                modelSSBO.flags |= (1u << 0);
        }
        if (Sol_Comp_Has(world, id, ScBuff))
        {
            if (Sol_Buff_HasBuff(world, id, BUFFKIND_INVULN))
                modelSSBO.flags |= (1u << 1);
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
            modelSSBO.flags |= (1u << 2);
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

Xform Sol_Model_GetBoneXform(World *world, int id, const char *name)
{
    Xform result = {0};

    ScModel *model = Sol_Comp_Get(world, id, ScModel);

    if (!model)
    {
        result.pos = Xform_GetDraw(world, id).pos;
        result.rot = Xform_GetDraw(world, id).rot;
        return result;
    }

    ScAnim *anim = Sol_Comp_Get(world, id, ScAnim);

    SolSkeleton *skeleton = &loaded_models[model->kind].skeleton;
    Xform xform           = Xform_GetDraw(world, id);

    // Find the bone by exact name.
    int boneIdx = -1;

    for (int i = 0; i < skeleton->boneCount; i++)
    {
        if (strcmp(skeleton->bones[i].name, name) == 0)
        {
            boneIdx = i;
            break;
        }
    }
    if(Sol_Comp_Has(world, id, ScWeapon))
    {
        sollog(boneIdx, skeleton->boneCount);
        
    }
    // Bone wasn't found.
    if (boneIdx < 0)
    {
        result.pos = xform.pos;
        result.rot = xform.rot;
        return result;
    }

    /*
     * Build the bone's model-space transform.
     *
     * With animation:
     *
     *     animatedPose * bindPose
     *
     * Without animation:
     *
     *     bindPose
     */
    mat4 bindPose;
    glm_mat4_inv(skeleton->bones[boneIdx].inverseBind, bindPose);

    mat4 boneModelSpace;

    if (anim)
    {
        glm_mat4_mul(anim->pose.bones[boneIdx], bindPose, boneModelSpace);
    }
    else
    {
        glm_mat4_copy(bindPose, boneModelSpace);
    }

    /*
     * Build the entity's world transform.
     */
    mat4 entityWorld;
    glm_mat4_identity(entityWorld);

    xform.pos.y += model_kinds[model->kind].y_offset;

    vec3 actualDrawPos = {xform.pos.x, xform.pos.y + (model->yOffset * xform.sca.y), xform.pos.z};

    glm_translate(entityWorld, actualDrawPos);

    glm_quat_rotate(entityWorld, xform.rot.raw, entityWorld);

    glm_scale(entityWorld, xform.sca.raw);

    /*
     * EntityWorld * BoneModelSpace
     */
    mat4 finalWorldMat;

    glm_mat4_mul(entityWorld, boneModelSpace, finalWorldMat);

    /*
     * Position.
     */
    result.pos.x = finalWorldMat[3][0];
    result.pos.y = finalWorldMat[3][1];
    result.pos.z = finalWorldMat[3][2];

    /*
     * Remove scale before extracting rotation.
     */
    mat4 rotationMat;
    glm_mat4_identity(rotationMat);

    glm_vec3_normalize_to(finalWorldMat[0], rotationMat[0]);

    glm_vec3_normalize_to(finalWorldMat[1], rotationMat[1]);

    glm_vec3_normalize_to(finalWorldMat[2], rotationMat[2]);

    glm_mat4_quat(rotationMat, result.rot.raw);

    return result;
}

float Sol_GetBoneRoll(vec3s tangent, vec3s boneUp)
{
    tangent     = vecNorm(tangent);
    vec3s refUp = WORLD_UP;

    if (fabsf(vecDot(tangent, refUp)) > 0.99f)
        refUp = WORLD_LEFT;

    vec3s side = vecNorm(vecCross(tangent, refUp));
    vec3s up   = vecNorm(vecCross(side, tangent));

    return atan2f(vecDot(boneUp, side), vecDot(boneUp, up));
}