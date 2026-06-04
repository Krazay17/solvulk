#include "sol_core.h"

#include "ability_i.h"

#define COOLDOWN 2.0f
#define DURATION 0.6f
#define VELOCITY 50.0f
#define ALPHAMOD 1.3f
#define DAMAGE_DELAY 0.0f
#define HITINTERVAL 0.01f

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
            SolRayResult results[256] = {0};
            int          hits = Sol_SphereCast(world, (SolRay){.pos = pos, .ignoreEnt = id}, 2.5f, results, 256);
            for (int i = 0; i < hits; i++)
            {
                CompCombat *combat = &world->combats[id];
                if (!Sol_Owner_GetHostile(world, id, results[i].entId))
                    continue;
                if (combat->hitEnts[results[i].entId])
                    continue;
                combat->hitEnts[results[i].entId] = true;
                if (world->masks[results[i].entId] & HAS_PROJECTILE)
                {
                    Sol_Physx_SetRedirectVel(world, results[i].entId, Sol_Controller_GetAimdir(world, id));
                    Sol_Owner_Add(world, results[i].entId, id);
                    continue;
                }
                
                Sol_Event_Add(world, (SolEvent){
                                         .kind        = EVENTKIND_HIT,
                                         .as.hit.entA = id,
                                         .as.hit.entB = results[i].entId,
                                         .as.hit.pos  = results[i].pos,
                                         .as.hit.kind = HITKIND_SHIELD_PULSE,
                                         .as.hit.vel  = vecSub(results[i].pos, pos),
                                     });

                Sol_Event_Add(world, (SolEvent){
                                         .kind       = EVENTKIND_FX,
                                         .as.fx.pos  = results[i].pos,
                                         .as.fx.kind = FXKIND_SPINHIT,
                                         .as.fx.entA = results[i].entId,
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
            .anim = ANIM_SPINSLASH, .layerId = ANIM_LAYER_OVERRIDE, .oneShot = true, .speed = 1.6f, .seek = 0.4f});
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

bool Spinslash_State_CanEnter(World *world, int id, u32 last, u32 next, u32 slot)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[slot];
    return !(data->lastExited + COOLDOWN > Sol_GetGameTime());
}
