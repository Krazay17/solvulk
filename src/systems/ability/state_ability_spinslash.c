#include "sol_core.h"

#include "ability_i.h"

#define DURATION 0.5f
#define VELOCITY 40.0f
#define ALPHAMOD 1.3f
#define DAMAGE_DELAY 0.0f
#define HITINTERVAL 0.05f

void Spinslash_State_Update(World *world, int id, float dt)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ability->activeSlot];
    data->elapsed += dt;
    data->accum += dt;
    if (data->elapsed >= DURATION)
    {
        Sol_Ability_SetState(world, id, ABILITY_STATE_IDLE, ability->activeSlot, false);
        return;
    }
    CompController *cont             = &world->controllers[id];
    world->movements[id].frictionMod = 0.0f;
    float alpha                      = ALPHAMOD - (data->elapsed / DURATION);
    Sol_Physx_SetVel(world, id, glms_vec3_scale(cont->lookdir, alpha * VELOCITY));
    vec3s vel = Sol_Physx_GetVel(world, id);

    // aoe scan over time, hit ents once
    if (data->elapsed > DAMAGE_DELAY)
    {
        if (data->accum > HITINTERVAL)
        {
            data->accum               = 0;
            vec3s pos                 = Sol_Xform_GetPos(world, id);
            pos                       = glms_vec3_add(pos, cont->lookdir);
            SolRay       ray          = {.pos = glms_vec3_add(pos, vecSca(Sol_Physx_GetVelDir(world, id), 1.0f)), .ignoreEnt = id};
            SolRayResult results[256] = {0};
            int          hits         = Sol_SphereCast(world, ray, 1.7f, results, 256);
            for (int i = 0; i < hits; i++)
            {
                CompCombat *combat = &world->combats[id];
                if (combat->hitEnts[results[i].entId])
                    continue;
                combat->hitEnts[results[i].entId] = true;
                Sol_Event_Add(world, (SolEvent){
                                         .kind              = EVENTKIND_HIT,
                                         .as.hit.entA       = id,
                                         .as.hit.entB       = results[i].entId,
                                         .as.hit.damage     = data->damage,
                                         .as.hit.effectMask = data->effects,
                                         .as.hit.buffMask   = data->buffs,
                                         .as.hit.pos        = results[i].pos,
                                         .as.hit.vel        = vecSub(results[i].pos, pos),
                                         .as.hit.fxKind     = FXKIND_SPINHIT,
                                     });
            }
        }
    }
}

void Spinslash_State_Enter(World *world, int id)
{
    CompAbility    *ability = &world->abilities[id];
    AbilityData    *data    = &ability->stateData[ability->activeSlot];
    CompController *cont    = &world->controllers[id];

    data->accum = HITINTERVAL;
    Sol_Combat_ClearHits(world, id);
    Ability_BoostToDir(world, id, cont->lookdir, VELOCITY);
    Sol_Model_PlayAnim(
        world, id,
        (AnimDesc){
            .anim = ANIM_SPINSLASH, .layerId = ANIM_LAYER_OVERRIDE, .playKind = ANIMPLAYKIND_ONESHOT, .speed = 1.8f, .seek = 0.5f});
}

void Spinslash_State_Exit(World *world, int id)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ability->activeSlot];

    data->lastExited = Sol_GetGameTime();

    Sol_Model_PlayAnim(world, id, (AnimDesc){.layerId = ANIM_LAYER_OVERRIDE});
}

bool Spinslash_State_CanExit(World *world, int id, u32 next)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ability->activeSlot];

    return data->elapsed > DURATION;
}

bool Spinslash_State_CanEnter(World *world, int id, u32 last, u32 next, int slot)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[slot];
    return !(data->lastExited + data->cooldown > Sol_GetGameTime());
}
