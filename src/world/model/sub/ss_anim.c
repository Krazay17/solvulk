/*
 * File: ss_anim.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-08-22
 *
 */

#include "model/si_model.h"
#include "sol_core.h"
#include "world.h"
#include "model.h"
#include "sol_math.h"

#include "xform/s_xform.h"
#include "movement/s_movement.h"
#include "ability/s_ability.h"
#include "physx/s_body.h"

const int strafe_map[STRAFE_COUNT] = {
    [STRAFE_FWD] = ANIM_WALK_FWD,       [STRAFE_FWD_LEFT] = ANIM_WALK_FWD,  [STRAFE_LEFT] = ANIM_WALK_LEFT,
    [STRAFE_BWD_LEFT] = ANIM_WALK_LEFT, [STRAFE_BWD] = ANIM_WALK_BWD,       [STRAFE_BWD_RIGHT] = ANIM_WALK_RIGHT,
    [STRAFE_RIGHT] = ANIM_WALK_RIGHT,   [STRAFE_FWD_RIGHT] = ANIM_WALK_FWD,
};
const int wallrun_map[WALLTOUCH_COUNT] = {
    [WALLTOUCH_FRONT] = ANIM_WALLRUN_FWD,
    [WALLTOUCH_LEFT]  = ANIM_WALLRUN_LEFT,
    [WALLTOUCH_BACK]  = ANIM_WALLRUN_LEFT,
    [WALLTOUCH_RIGHT] = ANIM_WALLRUN_RIGHT,
};
const int walljump_map[WALLTOUCH_COUNT] = {
    [WALLTOUCH_FRONT] = ANIM_BACKFLIP,
    [WALLTOUCH_LEFT]  = ANIM_WALLJUMP_LEFT,
    [WALLTOUCH_BACK]  = ANIM_FLIPJUMP,
    [WALLTOUCH_RIGHT] = ANIM_WALLJUMP_RIGHT,
};
const int crouch_map[STRAFE_COUNT] = {
    [STRAFE_FWD] = ANIM_CROUCHWALK_FWD,     [STRAFE_FWD_LEFT] = ANIM_CROUCHWALK_FWD,
    [STRAFE_LEFT] = ANIM_CROUCHWALK_LEFT,   [STRAFE_BWD_LEFT] = ANIM_CROUCHWALK_LEFT,
    [STRAFE_BWD] = ANIM_CROUCHWALK_BWD,     [STRAFE_BWD_RIGHT] = ANIM_CROUCHWALK_RIGHT,
    [STRAFE_RIGHT] = ANIM_CROUCHWALK_RIGHT, [STRAFE_FWD_RIGHT] = ANIM_CROUCHWALK_FWD,
};
const int dash_map[STRAFE_COUNT] = {
    [STRAFE_FWD] = ANIM_DASH_FWD,       [STRAFE_FWD_LEFT] = ANIM_DASH_LEFT,   [STRAFE_LEFT] = ANIM_DASH_LEFT,
    [STRAFE_BWD_LEFT] = ANIM_DASH_LEFT, [STRAFE_BWD] = ANIM_DASH_BWD,         [STRAFE_BWD_RIGHT] = ANIM_DASH_RIGHT,
    [STRAFE_RIGHT] = ANIM_DASH_RIGHT,   [STRAFE_FWD_RIGHT] = ANIM_DASH_RIGHT,
};

void Anim_Tick(World *world, double dt, double time)
{
    float fdt = (float)dt;

    WorldModels *wc = world->dense_components[WORLD_SYS_MODEL];
    for (int i = 0; i < wc->anim_cnt; i++)
    {
        int        id    = wc->anim_dense[i];
        CompAnim  *anim  = &wc->anims[i];
        CompModel *model = Sol_Model_Get(world, id);
        CompXform *xform = &world->xforms[id];

        CompAbility *ability = Sol_Ability_Get(world, id);
        if (ability)
        {
            AbilityStateData *data         = &ability->stateData[ability->activeSlot];
            AnimDesc          ability_anim = {.layerId = ANIM_LAYER_OVERRIDE};
            switch (ability->state)
            {
            case ABILITY_STATE_CLAW: {
                ability_anim.layerId = ANIM_LAYER_UPPER;
                ability_anim.anim    = ability->activeSlot == 1 ? ANIM_ATTACK_RIGHT : ANIM_ATTACK_LEFT;
                ability_anim.seek    = 0.16f;
            }
            break;
            case ABILITY_STATE_LASER: {
                ability_anim.layerId = ANIM_LAYER_UPPER;
                switch (data->stage)
                {
                case 0:
                    ability_anim.anim = ability->activeSlot == 1 ? ANIM_ATTACK_RIGHT : ANIM_ATTACK_LEFT;
                    break;
                case 1:
                    ability_anim.anim = ability->activeSlot == 1 ? ANIM_CHANNEL_RIGHT : ANIM_CHANNEL_LEFT;
                    break;
                }
            }
            break;
            case ABILITY_STATE_DASH: {
                ability_anim.anim  = dash_map[data->as.dash.strafe];
                ability_anim.speed = 1.1f - data->duration;
                ability_anim.seek  = 0.05f;
            }
            break;
            case ABILITY_STATE_FIREBALL: {
                ability_anim.layerId = ANIM_LAYER_UPPER;
                switch (data->stage)
                {
                case 0:
                    ability_anim.anim = ANIM_CHARGE_LEFT;
                    break;
                case 1:
                case 2:
                    ability_anim.anim = ANIM_ATTACK_LEFT;
                    ability_anim.seek = 0.16f;
                    break;
                }
            }
            break;
            }
            if (ability->state != 0)
                Sol_Model_PlayAnim(world, id, ability_anim);
        }

        if (WHasB(world, id, HAS_MOVEMENT))
        {
            CompMovement  *movement     = &world->movements[id];
            MoveStateData *data         = &movement->stateData[movement->state];
            bool           modify_speed = false;
            AnimDesc       move_anim    = {.anim = ANIM_IDLE, .layerId = ANIM_LAYER_BASE};
            switch (movement->state)
            {
            case MOVE_WALLRUN: {
                modify_speed   = true;
                move_anim.anim = wallrun_map[data->as.wallrun.wallTouch];
            }
            break;
            case MOVE_WALK: {
                modify_speed   = true;
                move_anim.anim = strafe_map[data->as.walk.strafe];
            }
            break;
            case MOVE_STUN: {
                move_anim.anim = ANIM_STUN;
            }
            break;
            case MOVE_FALL: {
                move_anim.anim = ANIM_FALL;
            }
            break;
            case MOVE_JUMP: {
                move_anim.anim = data->as.jump.airJump ? ANIM_FLIPJUMP : ANIM_JUMP;
            }
            break;
            case MOVE_CROUCH: {
                modify_speed   = true;
                move_anim.anim = crouch_map[data->as.crouch.strafe];
            }
            break;
            case MOVE_SLIDE: {
                move_anim.anim = ANIM_SLIDE_FWD;
            }
            break;
            case MOVE_WALLJUMP: {
                move_anim.anim = walljump_map[movement->stateData[MOVE_WALLRUN].as.wallrun.wallTouch];
            }
            break;
            case MOVE_MANTLE: {
                move_anim.playKind = ANIMPLAYKIND_NOLOOP;
                move_anim.anim     = data->as.mantle.doRoll ? ANIM_MANTLE_ROLL : ANIM_MANTLE;
                Sol_Model_SetAnimSpeed(world, id, move_anim.layerId, 2.5f - data->as.mantle.dist);
            }
            break;
            case MOVE_FLY: {
            }
            break;
            case MOVE_DEAD: {
                move_anim.anim     = ANIM_DEATH;
                move_anim.playKind = ANIMPLAYKIND_NOLOOP;
            }
            break;
            }
            Sol_Model_PlayAnim(world, id, move_anim);
            if (modify_speed)
                Sol_Model_SetAnimSpeed(world, id, move_anim.layerId,
                                       Sol_Physx_GetSpeed(world, id) / Sol_Movement_GetBaseSpeed(world, id));
        }
        for (int i = 0; i < ANIM_LAYER_COUNT; i++)
        {
            if (anim->layers[i].last_frame_played != solState.tickCounter)
                Sol_Model_StopAnim(world, id, i, BLEND_SPEED_DEFAULT);
        }
    }
}

void Anim_Solver(World *world, double dt, double time)
{
    float fdt = (float)dt;

    WorldModels *wc = world->dense_components[WORLD_SYS_MODEL];
    for (int i = 0; i < wc->anim_cnt; i++)
    {
        int       id   = wc->anim_dense[i];
        CompAnim *anim = &wc->anims[i];

        CompModel *model = Sol_Model_Get(world, id);
        if (!model || model->modelId < 0)
            continue;

        SolModel *m = &loaded_models[model->modelId];

        for (int L = 0; L < ANIM_LAYER_COUNT; L++)
        {
            AnimLayer *layer = &anim->layers[L];

            if (layer->currentAnim < 0 || layer->currentAnim >= m->skeleton.animationCount)
            {
                layer->currentAnim = -1;
                continue;
            }

            float dur     = m->skeleton.animations[layer->currentAnim].duration;
            float speed   = (layer->playRate != 0.0f) ? layer->playRate * fdt : fdt;
            float newSeek = layer->currentSeek + speed;

            switch (layer->playKind)
            {
            case ANIMPLAYKIND_ONESHOT: {
                layer->currentSeek = (newSeek < dur) ? newSeek : (dur - 0.001f);
                // Convert speed back to duration in seconds
                float blendOutDuration = (layer->blendOutSpeed > 0.0f) ? (1.0f / layer->blendOutSpeed) : 0.0f;
                float fadeOutTime      = fminf(blendOutDuration, dur * 0.5f);

                if (newSeek >= (dur - fadeOutTime) && !layer->isBlendingOut)
                {
                    layer->isBlendingOut = true;
                }
                break;
            }

            case ANIMPLAYKIND_NOLOOP:
                layer->currentSeek = (newSeek < dur) ? newSeek : (dur - 0.001f);
                break;

            default: // LOOP
                layer->currentSeek = fmodf(fmodf(newSeek, dur) + dur, dur);
                break;
            }

            if (layer->blendFactor < 1.0f)
            {
                layer->blendFactor += fdt * layer->blendInSpeed;
                if (layer->blendFactor > 1.0f)
                    layer->blendFactor = 1.0f;
            }

            // Layer Weight Fade-Out / Fade-In & Cleanup
            if (layer->isBlendingOut)
            {
                layer->weight -= fdt * layer->blendOutSpeed;

                if (layer->weight <= 0.0f)
                {
                    layer->weight        = 0.0f;
                    layer->currentAnim   = -1;
                    layer->animId        = -1;
                    layer->isBlendingOut = false;
                    layer->blendFactor   = 0.0f;
                }
            }
            else if (layer->weight < 1.0f)
            {
                float rampSpeed = (layer->blendInSpeed > 0.0f) ? layer->blendInSpeed : (1.0f / BLEND_SPEED_DEFAULT);
                layer->weight += fdt * rampSpeed;
                if (layer->weight > 1.0f)
                    layer->weight = 1.0f;
            }
        }

        Sol_Skeleton_Pose(model->modelId, &anim->pose, anim->layers, &anim->lastPose, &anim->hasLastPose);
    }
}

CompAnim *Sol_Model_AddAnim(WorldModels *wc, int id, int kind)
{
    if (loaded_models[kind].skeleton.animationCount < 1)
        return NULL;

    if (wc->anim_sparse[id] != -1)
        return &wc->anims[wc->anim_sparse[id]];

    if (wc->anim_cnt >= wc->anim_cap)
    {
        int old_cap    = wc->anim_cap;
        wc->anim_cap   = (wc->anim_cap == 0) ? 16 : wc->anim_cap * 2;
        wc->anim_dense = realloc(wc->anim_dense, sizeof(int) * wc->anim_cap);
        wc->anims      = realloc(wc->anims, sizeof(CompAnim) * wc->anim_cap);

        memset(&wc->anims[old_cap], 0, sizeof(CompAnim) * (wc->anim_cap - old_cap));
    }

    int denseIdx             = wc->anim_cnt++;
    wc->anim_sparse[id]      = denseIdx;
    wc->anim_dense[denseIdx] = id;
    CompAnim *anim_comp      = &wc->anims[denseIdx];

    for (int i = 0; i < ANIM_LAYER_COUNT; i++)
    {
        anim_comp->layers[i].currentAnim = -1;
        anim_comp->layers[i].animId      = -1;
        anim_comp->layers[i].currentSeek = 0.0f;
        anim_comp->layers[i].blendFactor = 1.0f;
        anim_comp->layers[i].weight      = 1.0f;
    }

    // Assign base layer default animation
    anim_comp->layers[0].animId      = 0;
    anim_comp->layers[0].currentAnim = 0;
    anim_comp->hasLastPose           = false;
    return anim_comp;
}

CompAnim *Sol_Model_GetAnim(World *world, int id)
{
    WorldModels *wc = world->dense_components[WORLD_SYS_MODEL];
    if (wc->anim_sparse[id] != -1)
        return &wc->anims[wc->anim_sparse[id]];
    return NULL;
}

void Sol_Model_RemAnim(WorldModels *wc, int id)
{
    int idx = wc->anim_sparse[id];
    if (idx < 0)
        return;

    int lastIdx             = wc->anim_cnt - 1;
    int lastId              = wc->anim_dense[lastIdx];
    wc->anims[idx]          = wc->anims[lastIdx];
    wc->anim_dense[idx]     = lastId;
    wc->anim_sparse[lastId] = idx;
    wc->anim_sparse[id]     = -1;
    wc->anim_cnt--;
}

bool Sol_Model_HasAnim(World *world, int id)
{
    WorldModels *wc = world->dense_components[WORLD_SYS_MODEL];
    return wc->anim_sparse[id] != -1;
}