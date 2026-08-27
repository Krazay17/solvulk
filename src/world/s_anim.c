#include "world.h"
#include "model.h"
#include "sol_core.h"

static void Anim_Solver(World *world, double dt, double time)
{
    float fdt = (float)dt;

    SparseSet_SolAnim *set = Sol_Comp_Set(world, SolAnim);
    for (int i = 0; i < set->cnt; i++)
    {
        int      id   = set->dense[i];
        SolAnim *anim = &set->data[i];

        SolModel *model = Sol_Comp_Get(world, id, SolModel);
        if (!model || model->modelId < 0)
            continue;

        SolModelData *m = &loaded_models[model->modelId];

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

void Anim_Tick(World *world, double dt, double time)
{
    float fdt = (float)dt;

    SparseSet_SolAnim *set = Sol_Comp_Set(world, SolAnim);
    for (int i = 0; i < set->cnt; i++)
    {
        int      id   = set->dense[i];
        SolAnim *anim = &set->data[i];

        if (Sol_Comp_Has(world, id, SolAbility))
        {
            SolAbility       *ability      = Sol_Comp_Get(world, id, SolAbility);
            AbilityStateData *data         = &ability->stateData[ability->activeSlot];
            AnimDesc          ability_anim = {.layerId = ANIM_LAYER_OVERRIDE};
            switch (ability->state)
            {
            case ABILITY_STATE_DASH: {
                ability_anim.anim  = dash_map[data->as.dash.strafe];
                ability_anim.speed = 1.1f - data->duration;
                ability_anim.seek  = 0.05f;
            }
            break;
            case ABILITY_STATE_CLAW: {
                ability_anim.layerId = ANIM_LAYER_UPPER;
                ability_anim.anim    = ability->activeSlot == 1 ? ANIM_ATTACK_RIGHT : ANIM_ATTACK_LEFT;
                ability_anim.seek    = 0.16f;
            }
            break;
            case ABILITY_STATE_FIREBALL: {
                ability_anim.layerId = ANIM_LAYER_UPPER;
                switch (data->stage)
                {
                case 0:
                    ability_anim.anim = ability->activeSlot == 1 ? ANIM_CHARGE_RIGHT : ANIM_CHARGE_LEFT;
                    break;
                case 1:
                case 2:
                    ability_anim.anim = ability->activeSlot == 1 ? ANIM_ATTACK_RIGHT : ANIM_ATTACK_LEFT;
                    ability_anim.seek = 0.16f;
                    break;
                }
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
            }
            if (ability->state != 0)
                Sol_Anim_Play(world, id, ability_anim);
        }

        if (Sol_Comp_Has(world, id, SolMovement))
        {
            SolMovement   *movement     = Sol_Comp_Get(world, id, SolMovement);
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
                Sol_Anim_SetSpeed(world, id, move_anim.layerId, 2.5f - data->as.mantle.dist);
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
            Sol_Anim_Play(world, id, move_anim);
            // if (modify_speed)
            //     Sol_Anim_SetSpeed(world, id, move_anim.layerId,
            //                            Sol_Physx_GetSpeed(world, id) / Sol_Movement_GetBaseSpeed(world, id));
        }
        else
        {
            Sol_Anim_Play(world, id, (AnimDesc){.anim = ANIM_IDLE, .layerId = ANIM_LAYER_BASE});
        }
        for (int i = 0; i < ANIM_LAYER_COUNT; i++)
        {
            if (anim->layers[i].last_frame_played != world->currentTick)
                Sol_Anim_Stop(world, id, i, BLEND_SPEED_DEFAULT);
        }
    }

    Anim_Solver(world, dt, time);
}

SolAnim *Sol_Anim_Add(World *world, int id)
{
    SolAnim *anim_comp = Sol_Comp_Add(world, id, SolAnim);
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

void Sol_Anim_Play(World *world, int id, AnimDesc desc)
{
    SolModel  *modelComp = Sol_Comp_Get(world, id, SolModel);
    SolAnim   *anim      = Sol_Comp_Get(world, id, SolAnim);
    AnimLayer *layer     = &anim->layers[desc.layerId];

    AnimId animId   = desc.anim;
    float  blendIn  = desc.blendIn > 0.0f ? desc.blendIn : BLEND_SPEED_DEFAULT;
    float  blendOut = desc.blendOut > 0.0f ? desc.blendOut : BLEND_SPEED_DEFAULT;

    layer->last_frame_played = world->currentTick;

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

void Sol_Anim_Stop(World *world, int id, AnimLayerId layerId, float blendOut)
{
    AnimLayer *layer = &Sol_Comp_Get(world, id, SolAnim)->layers[layerId];
    if (layer->currentAnim == -1 || layer->isBlendingOut)
        return;

    float dur            = blendOut > 0.0f ? blendOut : 0.25f;
    layer->isBlendingOut = true;
    layer->blendOutSpeed = 1.0f / dur;
}

void Sol_Anim_SetSpeed(World *world, int id, AnimLayerId layerId, float rate)
{
    AnimLayer *layer = &Sol_Comp_Get(world, id, SolAnim)->layers[layerId];
    if (layer->currentAnim < 0)
        return;
    layer->playRate = rate;
}

void Sol_Anim_SetSeek(World *world, int id, AnimLayerId layerId, float seek)
{
    AnimLayer *layer = &Sol_Comp_Get(world, id, SolAnim)->layers[layerId];
    if (layer->currentAnim < 0)
        return;
    layer->currentSeek = seek;
}
