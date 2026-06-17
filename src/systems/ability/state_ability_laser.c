#include "sol_core.h"

#include "ability_i.h"

#define HITINTERVAL 0.1f
#define HITDELAY 0.25f
#define LASER_LENGTH 15.0f
#define MAX_DURATION 3.5f
#define MAX_CHARGE 2.0f
#define MIN_CHARGE 0.1f

static bool Leave_State(World *world, int id, CompAbility *ability)
{
    if (!ability->stateData[ability->activeSlot].held)
        if (Sol_Ability_SetState(world, id, ABILITY_STATE_IDLE, 0, true))
            return true;
    return false;
}

void Laser_State_Update(World *world, int id, float dt)
{
    CompAbility *ability = &world->abilities[id];
    if (Leave_State(world, id, ability))
        return;
    AbilityData *data = &ability->stateData[ability->activeSlot];
    data->elapsed += dt;

    // if (data->elapsed >= data->duration)
    // {
    //     Sol_Ability_SetState(world, id, ABILITY_STATE_IDLE, 0, 1);
    //     return;
    // }

    CompController *controller = &world->controllers[id];

    SolRay lengthCheck = {
        .dir       = controller->aimdir,
        .dist      = LASER_LENGTH,
        .ignoreEnt = id,
        .pos       = controller->aimpos,
    };
    SolRayResult lengthResult = Sol_Raycast(world, lengthCheck);
    if (Sol_Physx_GetVel(world, id).y < 0.0f && (Sol_Movement_GetState(world, id) == MOVE_FALL))
        Sol_Physx_SetVelY(world, id, fmax(Sol_Physx_GetVel(world, id).y, 0.0f));

    if (data->elapsed < HITDELAY)
        return;

    if (data->stage < 1)
    {
        data->stage   = 1;
        AnimDesc desc = {.anim     = ability->activeSlot == 1 ? ANIM_CHANNEL_RIGHT : ANIM_CHANNEL_LEFT,
                         .layerId  = ANIM_LAYER_UPPER,
                         .speed    = 1.0f,
                         .playKind = ANIMPLAYKIND_LOOP,
                         .blendIn  = 0.1f};
        Sol_Model_PlayAnim(world, id, desc);
    }

    if (data->stage == 1)
    {
        data->charge += dt * 10.0f;
        if (data->charge >= MAX_CHARGE)
            data->stage = 2;
    }
    else
        data->charge -= dt * 6.0f;

    data->charge = fminf(MAX_CHARGE, fmaxf(MIN_CHARGE, data->charge));

    data->accum += dt;
    if (data->accum < HITINTERVAL)
        return;
    data->accum        = 0;
    CompCombat *combat = &world->combats[id];
    float finalDamage = (data->damage * data->charge) * HITINTERVAL;

    
    Sol_Combat_ClearHits(world, id);
    SolRay ray = {
        .dir       = controller->aimdir,
        .dist      = lengthResult.dist,
        .ignoreEnt = id,
        .pos       = controller->aimpos,
        .mask      = 0b1,
    };

    SolRayResult results[64];
    int          hits = Sol_SphereCast(world, ray, Sol_Math_Lerp(0.1f, 0.77f, data->charge / MAX_CHARGE), results, 64);

    for (int i = 0; i < hits; i++)
    {
        SolRayResult result = results[i];

        float dot =
            glms_vec3_dot(controller->aimdir, glms_vec3_normalize(glms_vec3_sub(result.pos, controller->aimpos)));
        if (dot < 0)
            continue;
        if (combat->hitEnts[result.entId])
            continue;
        combat->hitEnts[result.entId] = true;

        Sol_Event_Add(world, (SolEvent){
                                 .kind              = EVENTKIND_HIT,
                                 .as.hit.damage     = finalDamage,
                                 .as.hit.buffMask   = data->buffs,
                                 .as.hit.effectMask = data->effects,
                                 .as.hit.entA       = id,
                                 .as.hit.entB       = result.entId,
                                 .as.hit.pos        = result.pos,
                                 .as.hit.vel        = controller->aimdir,
                                 .as.hit.power      = data->charge,
                             });
    }
}

void Laser_State_Enter(World *world, int id)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ability->activeSlot];
    CompCombat  *combat  = &world->combats[id];
    data->accum          = HITINTERVAL;
    data->charge         = 0;
    data->stage          = 0;
    Sol_Combat_ClearHits(world, id);

    Sol_World_Audio_Add(world, id, SOL_AUDIO_LASER, 0.3f, 0);

    float animRate           = 1.2f;
    combat->baseAnimRate     = animRate;
    combat->hitPause         = 0;
    combat->hitPauseDiminish = 0;

    AnimDesc desc = {.anim     = ability->activeSlot == 1 ? ANIM_ATTACK_RIGHT : ANIM_ATTACK_LEFT,
                     .layerId  = ANIM_LAYER_UPPER,
                     .speed    = animRate,
                     .playKind = ANIMPLAYKIND_LOOP,
                     .blendIn  = 0.05f};

    Sol_Model_PlayAnim(world, id, desc);
}

void Laser_State_Exit(World *world, int id)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ability->activeSlot];
    data->lastExited     = Sol_GetGameTime();
    Sol_Model_PlayAnim(world, id, (AnimDesc){.layerId = ANIM_LAYER_UPPER});
    Sol_World_Audio_Remove(world, id, 0);
}

bool Laser_State_CanExit(World *world, int id, u32 next)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ability->activeSlot];
    return true; // data->elapsed >= data->duration * 0.8f;
}

bool Laser_State_CanEnter(World *world, int id, u32 last, u32 next, int slot)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[slot];
    return slot != ability->activeSlot && !(data->lastExited + data->cooldown > Sol_GetGameTime());
}
