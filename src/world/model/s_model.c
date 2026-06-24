#include "s_model.h"
#include "sol_core.h"
#include "sol_math.h"
#include "world.h"
#include "model.h"
#include "xform/s_xform.h"
#include "buff/s_buff.h"
#include "render/render.h"
#include "vital/s_vital.h"

#define ONESHOT_FADE_DURATION 0.25f

void Model_Draw(World *world, double dt, double time);

void Sol_Model_Init(World *world)
{
    world->models = calloc(MAX_ENTS, sizeof(CompModel));
    WAdd3d(world) = Model_Draw;
}

CompModel *Sol_Model_Add(World *world, int id, SolModelKind kind, float height)
{
    CompModel model = model_kinds[kind];
    model.modelId   = kind;
    model.yOffset   = -height * 0.5f;

    SolModel *m = Sol_GetModel(kind);
    if (m->skeleton.animationCount > 0)
    {
        model.hasAnim = true;
        for (int i = 0; i < ANIM_LAYER_COUNT; i++)
        {
            model.layers[i].currentAnim = -1;
            model.layers[i].currentSeek = 0;
            model.layers[i].blendFactor = 0;
        }
    }
    world->models[id] = model;
    world->masks[id] |= HAS_MODEL;

    Sol_Model_PlayAnim(world, id, (AnimDesc){.anim = ANIM_IDLE, .layerId = ANIM_LAYER_BASE});

    return &world->models[id];
}

void Model_Draw(World *world, double dt, double time)
{
    float fdt      = (float)dt;
    int   required = HAS_ACTIVE | HAS_MODEL;

    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;

        CompXform *xform     = &world->xforms[id];
        CompModel *modelComp = &world->models[id];
        if (modelComp->modelId < 0)
            continue;

        ModelSSBO modelSSBO = {0};
        modelSSBO.color = modelComp->color;
        if (world->masks[id] & HAS_INTERACT)
            if (Sol_Interact_GetState(world, id) & INTERACT_HOVERED || world->flags[id].flags & EFLAG_PICKEDUP)
                modelSSBO.flags |= (1 << 0);

        if (Sol_Buff_HasBuff(world, id, BUFFKIND_INVULN))
            modelSSBO.flags |= (1 << 1);

        if (world->masks[id] & HAS_VITAL)
            modelSSBO.hitTime = Sol_Vital_GetLastHitTime(world, id);
        else
            modelSSBO.hitTime = -100.0f;

        vec3s drawPos = xform->drawPos;
        if (modelComp->is2d)
        {
            modelSSBO.flags |= (1 << 2);
            drawPos.y += modelComp->yOffset;
            modelSSBO.position = (vec4s){UISCALE(drawPos.x + modelComp->xOffset), UISCALE(drawPos.y), drawPos.z, 1.0f};
            modelSSBO.rotation = (vec4s){xform->drawQuat.x, xform->drawQuat.y, xform->drawQuat.z, xform->drawQuat.w};
            modelSSBO.scale =
                (vec4s){UISCALE(xform->drawScale.x), UISCALE(xform->drawScale.y), UISCALE(xform->drawScale.z), 1.0f};
        }
        else
        {
            drawPos.y += modelComp->yOffset;
            modelSSBO.position = (vec4s){drawPos.x, drawPos.y, drawPos.z, 1.0f};
            modelSSBO.rotation = (vec4s){xform->drawQuat.x, xform->drawQuat.y, xform->drawQuat.z, xform->drawQuat.w};
            modelSSBO.scale    = (vec4s){xform->drawScale.x, xform->drawScale.y, xform->drawScale.z, 1.0f};
        }

        if (!modelComp->hasAnim)
        {
            Sol_Render_GetNext_Model(modelComp->modelId, &modelSSBO, NULL);
            continue;
        }
        BonesSSBO bonesSSBO = {0};
        SolModel *m         = Sol_GetModel(modelComp->modelId);

        for (int L = 0; L < ANIM_LAYER_COUNT; L++)
        {
            AnimLayer *layer = &modelComp->layers[L];
            if (layer->currentAnim < 0)
                continue;

            float dur     = m->skeleton.animations[layer->currentAnim].duration;
            float speed   = layer->playRate != 0 ? layer->playRate * fdt : fdt;
            float newSeek = layer->currentSeek + speed;
            switch (layer->playKind)
            {
            case ANIMPLAYKIND_NOLOOP:
                layer->currentSeek = newSeek < dur ? newSeek : dur - 0.001f;
                break;
            case ANIMPLAYKIND_ONESHOT:
                layer->currentSeek = newSeek < dur ? newSeek : dur - 0.001f;
                if (newSeek >= dur - ONESHOT_FADE_DURATION)
                {
                    if (!layer->fadeOut)
                    {
                        layer->fadeOut      = 1.0f;
                        layer->fadeOutSpeed = 1.0f / ONESHOT_FADE_DURATION;
                    }
                }
                break;
            default:
                layer->currentSeek = fmodf(fmodf(newSeek, dur) + dur, dur);
            }

            if (layer->lastAnim >= 0 && layer->blendFactor < 1.0f)
            {
                float lastDur   = m->skeleton.animations[layer->lastAnim].duration;
                layer->lastSeek = fmodf(layer->lastSeek + fdt, lastDur);
                layer->blendFactor += fdt * layer->blendSpeed;
                if (layer->blendFactor >= 1.0f)
                {
                    layer->blendFactor = 1.0f;
                    layer->lastAnim    = 0;
                }
            }
            // Fade-out countdown
            if (layer->fadeOut > 0.0f)
            {
                layer->fadeOut -= fdt * layer->fadeOutSpeed;
                if (layer->fadeOut <= 0.0f)
                {
                    layer->fadeOut     = 0.0f;
                    layer->lastAnim    = 0;
                    layer->lastSeek    = 0;
                    layer->blendFactor = 0;
                    layer->currentAnim = -1;
                    layer->animId      = 0;
                }
            }
        }

        PoseRequest req = {.outBones = bonesSSBO.bones};

        for (int L = 0; L < ANIM_LAYER_COUNT; L++)
        {
            req.layers[L] = (AnimBlend){
                .anim        = modelComp->layers[L].currentAnim,
                .lastAnim    = modelComp->layers[L].lastAnim,
                .seek        = modelComp->layers[L].currentSeek,
                .lastSeek    = modelComp->layers[L].lastSeek,
                .blendFactor = modelComp->layers[L].blendFactor,
            };
            req.masks[L]       = model_masks[modelComp->modelId].layers[L];
            req.layerWeight[L] = (modelComp->layers[L].fadeOut > 0.0f) ? modelComp->layers[L].fadeOut : 1.0f;
        }
        Sol_Skeleton_Pose(&m->skeleton, &req);
        memcpy(modelComp->bones, bonesSSBO.bones, sizeof(modelComp->bones));
        Sol_Render_GetNext_Model(modelComp->modelId, &modelSSBO, &bonesSSBO);
    }
}

void Sol_Model_PlayAnim(World *world, int id, AnimDesc desc)
{
    CompModel *modelComp = &world->models[id];
    AnimLayer *layer     = &modelComp->layers[desc.layerId];

    AnimId animId   = desc.anim;
    bool   force    = desc.playKind == ANIMPLAYKIND_ONESHOT || desc.force;
    float  seek     = desc.seek;
    float  speed    = desc.speed ? desc.speed : 1.0f;
    float  blendIn  = desc.blendIn ? desc.blendIn : 0.25f;
    float  blendOut = desc.blendOut ? desc.blendOut : 0.25f;
    if (layer->animId == animId && !force)
        return;
    if (animId < 1)
    {
        if (layer->animId == animId || layer->fadeOut > 0.0f)
            return;
        layer->fadeOut      = 1.0f;
        layer->fadeOutSpeed = 1.0f / blendOut;
        return;
    }
    i32 mappedAnim = model_anim_map[modelComp->modelId][animId];
    layer->animId  = animId;

    if (layer->blendFactor > 0.5f)
    {
        layer->lastAnim    = layer->currentAnim;
        layer->lastSeek    = layer->currentSeek;
        layer->blendFactor = 0;
    }
    layer->currentAnim = mappedAnim;
    layer->playRate    = speed;
    layer->currentSeek = seek;

    layer->blendSpeed   = 1.0f / blendIn;
    layer->fadeOutSpeed = 1.0f / blendOut;
    layer->fadeOut      = 0.0f;
    layer->playKind     = desc.playKind;
}

void Sol_Model_SetModelId(World *world, int id, int modelId)
{
    world->models[id].modelId = modelId;
}
SolModelKind Sol_Model_GetModelId(World *world, int id)
{
    return world->models[id].modelId;
}

float Sol_Model_GetAnimSpeed(World *world, int id, AnimLayerId layerId)
{
    AnimLayer *layer = &world->models[id].layers[layerId];
    return layer->playRate;
}

void Sol_Model_SetAnimSpeed(World *world, int id, AnimLayerId layerId, float rate)
{
    AnimLayer *layer = &world->models[id].layers[layerId];
    if (layer->currentAnim < 0)
        return;
    layer->playRate = rate;
}

void Sol_Model_SetAnimSeek(World *world, int id, AnimLayerId layerId, float seek)
{
    AnimLayer *layer = &world->models[id].layers[layerId];
    if (layer->currentAnim < 0)
        return;
    layer->currentSeek = seek;
}

void Sol_Model_SetTint(World *world, int id, vec4s color)
{
}

SolXform Sol_Model_GetBoneXform(World *world, int id, const char *name)
{
    SolXform     result   = {0};
    CompModel   *model    = &world->models[id];
    SolSkeleton *skeleton = &loaded_models[model->modelId].skeleton;
    CompXform   *xform    = &world->xforms[id];

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
    glm_mat4_mul(model->bones[boneIdx], bindPose, boneModelSpace);

    // 2. Construct the Entity's World Transform Matrix
    mat4 entityWorld;
    glm_mat4_identity(entityWorld);

    // Apply position (including your yOffset adjustment)
    vec3 actualDrawPos = {xform->drawPos.x, xform->drawPos.y + model->yOffset, xform->drawPos.z};
    glm_translate(entityWorld, actualDrawPos);

    // Apply rotation
    glm_quat_rotate(entityWorld, xform->drawQuat.raw, entityWorld);

    // Apply scale
    glm_scale(entityWorld, xform->drawScale.raw);

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

float Sol_Model_GetOffsetY(World *world, int id)
{
    return world->models[id].yawOffset;
}
void Sol_Model_SetOffsetY(World *world, int id, float offset)
{
    CompModel *model = &world->models[id];
    model->yOffset   = offset;
}
void Sol_Model_StopAnim(World *world, int id, AnimLayerId layerId)
{
    world->models[id].layers[layerId].fadeOut      = 1.0f;
    world->models[id].layers[layerId].fadeOutSpeed = 4.0f;
    world->models[id].layers[layerId].animId       = 0;
}