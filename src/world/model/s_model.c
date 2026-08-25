/*
 * File: s_model.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-08-17
 *
 */

#include "si_model.h"
#include "sol_core.h"
#include "sol_math.h"
#include "world.h"
#include "model.h"
#include "profiler.h"
#include "render/render.h"
#include "platform/platform.h"

#include "xform/s_xform.h"
#include "interact/s_interact.h"
#include "buff/s_buff.h"
#include "combat/s_combat.h"

static SolProfiler prof_tick = {.name = "ModelTick"};
static SolProfiler prof_draw = {.name = "ModelDraw"};

static void Model_Draw(World *world, double dt, double time)
{
    float fdt = (float)dt;

    WorldModels *wc = world->dense_components[WORLD_SYS_MODEL];
    for (int i = 0; i < wc->cnt; i++)
    {
        int        id        = wc->dense[i];
        CompModel *modelComp = &wc->models[i];

        CompXform *xform = &world->xforms[id];

        ModelSSBO modelSSBO = {0};
        modelSSBO.color     = modelComp->color;
        if (world->masks[id] & BITC(HAS_INTERACT))
            if (Sol_Interact_GetState(world, id) & (INTERACT_HOVERED | INTERACT_DRAGGING))
                modelSSBO.flags |= (1 << 0);

        if (Sol_Buff_HasBuff(world, id, BUFFKIND_INVULN))
            modelSSBO.flags |= (1 << 1);

        if (world->masks[id] & BITC(HAS_COMBAT))
            modelSSBO.hitTime = world->combats[id].lastHitTime;
        else
            modelSSBO.hitTime = -100.0f;

        vec3s drawPos = xform->drawPos;
        if (modelComp->is2d)
        {
            modelSSBO.flags |= (1 << 2);
            float px           = UISCALE(drawPos.x + (modelComp->xOffset * xform->scale.x));
            float py           = UISCALE(drawPos.y + (-modelComp->yOffset * xform->scale.y));
            float pz           = drawPos.z;
            modelSSBO.position = (vec4s){px, py, pz, 1.0f};
            modelSSBO.rotation = (vec4s){xform->drawQuat.x, xform->drawQuat.y, xform->drawQuat.z, xform->drawQuat.w};
            modelSSBO.scale =
                (vec4s){UISCALE(xform->drawScale.x), UISCALE(xform->drawScale.y), UISCALE(xform->drawScale.z), 1.0f};
        }
        else
        {
            drawPos.y += (modelComp->yOffset * xform->scale.y);
            modelSSBO.position = (vec4s){drawPos.x, drawPos.y, drawPos.z, 1.0f};
            modelSSBO.rotation = (vec4s){xform->drawQuat.x, xform->drawQuat.y, xform->drawQuat.z, xform->drawQuat.w};
            modelSSBO.scale    = (vec4s){xform->drawScale.x, xform->drawScale.y, xform->drawScale.z, 1.0f};
        }

        if (Sol_Model_HasAnim(world, id))
        {
            Sol_Render_GetNext_Model(modelComp->modelId, &modelSSBO, &Sol_Model_GetAnim(world, id)->pose);
        }
        else
        {
            Sol_Render_GetNext_Model(modelComp->modelId, &modelSSBO, NULL);
        }
    }
}

void Sol_Model_Init(World *world)
{
    WorldModels *wc                          = malloc(sizeof(WorldModels));
    world->dense_components[WORLD_SYS_MODEL] = wc;

    wc->cap    = 64;
    wc->cnt    = 0;
    wc->sparse = malloc(sizeof(int) * MAX_ENTS);
    wc->dense  = malloc(sizeof(int) * wc->cap);
    wc->models = malloc(sizeof(CompModel) * wc->cap);
    memset(wc->sparse, -1, sizeof(int) * MAX_ENTS);

    wc->anim_cap    = 32;
    wc->anim_cnt    = 0;
    wc->anim_sparse = malloc(sizeof(int) * MAX_ENTS);
    wc->anim_dense  = malloc(sizeof(int) * wc->anim_cap);
    wc->anims       = malloc(sizeof(CompAnim) * wc->anim_cap);
    memset(wc->anim_sparse, -1, sizeof(int) * MAX_ENTS);

    WAddTick(world) = Anim_Tick;
    WAddTick(world) = Anim_Solver;
    WAdd3d(world)   = Model_Draw;
}

CompModel *Sol_Model_Add(World *world, int id, int kind)
{
    WorldModels *wc = world->dense_components[WORLD_SYS_MODEL];
    if (!wc)
    {
        char buffer[64];
        sprintf(buffer, "WorldModels not initialized at world %d", world->kind);
        Sol_MessageBox(buffer, "Warning");
        return NULL;
    }
    if (wc->sparse[id] != -1)
        return &wc->models[wc->sparse[id]];
    if (wc->cnt >= wc->cap)
    {
        wc->cap *= 2;
        wc->dense  = realloc(wc->dense, sizeof(int) * wc->cap);
        wc->models = realloc(wc->models, sizeof(CompModel) * wc->cap);
    }
    int        denseIdx   = wc->cnt++;
    CompModel *model_comp = &wc->models[denseIdx];
    wc->sparse[id]        = denseIdx;
    wc->dense[denseIdx]   = id;
    *model_comp           = model_kinds[kind];
    model_comp->modelId   = kind;

    Sol_Model_AddAnim(wc, id, kind);

    return model_comp;
}

CompModel *Sol_Model_Get(World *world, int id)
{
    WorldModels *wc = world->dense_components[WORLD_SYS_MODEL];
    if (wc->sparse[id] != -1)
        return &wc->models[wc->sparse[id]];
    return NULL;
}

bool Sol_Model_Has(World *world, int id)
{
    WorldModels *wc = world->dense_components[WORLD_SYS_MODEL];
    return wc->sparse[id] != -1;
}

void Sol_Model_Rem(World *world, int id)
{
    WorldModels *wc  = world->dense_components[WORLD_SYS_MODEL];
    int          idx = wc->sparse[id];
    if (idx < 0)
        return;

    int lastIdx        = wc->cnt - 1;
    int lastId         = wc->dense[lastIdx];
    wc->models[idx]    = wc->models[lastIdx];
    wc->dense[idx]     = lastId;
    wc->sparse[lastId] = idx;
    wc->sparse[id]     = -1;
    wc->cnt--;
    Sol_Model_RemAnim(wc, id);
}

void Sol_Model_PlayAnim(World *world, int id, AnimDesc desc)
{
    CompModel *modelComp = Sol_Model_Get(world, id);
    CompAnim  *anim      = Sol_Model_GetAnim(world, id);
    AnimLayer *layer     = &anim->layers[desc.layerId];

    AnimId animId   = desc.anim;
    float  blendIn  = desc.blendIn > 0.0f ? desc.blendIn : BLEND_SPEED_DEFAULT;
    float  blendOut = desc.blendOut > 0.0f ? desc.blendOut : BLEND_SPEED_DEFAULT;

    layer->last_frame_played = solState.tickCounter;

    if (layer->animId == animId && !desc.force)
        return;

    if (animId < 0)
    {
        if (layer->animId == animId || layer->isBlendingOut)
            return;
        layer->isBlendingOut = true;
        layer->blendOutSpeed = 1.0f / blendOut;
        return;
    }

    bool wasActive = (layer->currentAnim != -1);

    layer->animId      = animId;
    layer->currentAnim = model_anim_map[modelComp->modelId][animId];
    layer->playRate    = desc.speed ? desc.speed : 1.0f;
    layer->currentSeek = desc.seek;
    layer->playKind    = desc.playKind;

    layer->blendInSpeed  = 1.0f / blendIn;
    layer->blendOutSpeed = 1.0f / blendOut;
    layer->isBlendingOut = false;

    // Snapshot the entire last-composited pose into the layer's transition cache
    if (anim->hasLastPose)
    {
        for (int i = 0; i < MAX_BONES; i++)
        {
            glm_vec3_copy(anim->lastPose.poseT[i].raw, layer->cachedT[i].raw);
            glm_vec4_copy(anim->lastPose.poseR[i].raw, layer->cachedR[i].raw);
            glm_vec3_copy(anim->lastPose.poseS[i].raw, layer->cachedS[i].raw);
        }
        layer->blendFactor = 0.0f;
    }
    else
    {
        layer->blendFactor = 1.0f;
    }

    if (!wasActive)
        layer->weight = 0.0f;
}

float Sol_Model_GetAnimSpeed(World *world, int id, AnimLayerId layerId)
{
    AnimLayer *layer = &Sol_Model_GetAnim(world, id)->layers[layerId];
    return layer->playRate;
}

void Sol_Model_SetAnimSpeed(World *world, int id, AnimLayerId layerId, float rate)
{
    AnimLayer *layer = &Sol_Model_GetAnim(world, id)->layers[layerId];
    if (layer->currentAnim < 0)
        return;
    layer->playRate = rate;
}

void Sol_Model_SetAnimSeek(World *world, int id, AnimLayerId layerId, float seek)
{
    AnimLayer *layer = &Sol_Model_GetAnim(world, id)->layers[layerId];
    if (layer->currentAnim < 0)
        return;
    layer->currentSeek = seek;
}

SolXform Sol_Model_GetBoneXform(World *world, int id, const char *name)
{
    SolXform     result   = {0};
    CompModel   *model    = Sol_Model_Get(world, id);
    CompAnim    *anim     = Sol_Model_GetAnim(world, id);
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
    glm_mat4_mul(anim->pose.bones[boneIdx], bindPose, boneModelSpace);

    // 2. Construct the Entity's World Transform Matrix
    mat4 entityWorld;
    glm_mat4_identity(entityWorld);

    // Apply position (including your yOffset adjustment)
    vec3 actualDrawPos = {xform->drawPos.x, xform->drawPos.y + (model->yOffset * xform->scale.y), xform->drawPos.z};
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

void Sol_Model_StopAnim(World *world, int id, AnimLayerId layerId, float blendOut)
{
    AnimLayer *layer = &Sol_Model_GetAnim(world, id)->layers[layerId];
    if (layer->currentAnim == -1 || layer->isBlendingOut)
        return;

    float dur = blendOut > 0.0f ? blendOut : 0.25f;
    layer->isBlendingOut = true;
    layer->blendOutSpeed = 1.0f / dur;
}