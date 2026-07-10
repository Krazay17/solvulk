#include "ability_i.h"
#include "sol_core.h"
#include "world.h"
#include "sol_math.h"
#include "audio.h"
#include "audio/s_audio.h"
#include "xform/s_xform.h"
#include "controller/s_controller.h"
#include "combat/s_combat.h"
#include "model/s_model.h"
#include "event/s_event.h"
#include "physx/s_body.h"
#include "movement/s_movement.h"

#define HITINTERVAL 0.1f
#define HITDELAY 0.25f
#define MAX_DURATION 3.5f
#define MAX_CHARGE 2.0f
#define MIN_CHARGE 0.2f

// static void Laser(World *world, int id, vec3s pos, vec3s dir)
// {
//     CompAbility *ability = &world->abilities[id];
//     AbilityData *data    = &ability->stateData[ability->activeSlot];
//     if (data->as.laser.laserPointCount >= MAX_Whip_BOUNCES * 2)
//         return;
//     Sol_Combat_ClearHits(world, id);
//     CompCombat *combat = &world->combats[id];
//     vec3s      *pointA = &data->as.laser.laserPoints[data->as.laser.laserPointCount++];
//     vec3s      *pointB = &data->as.laser.laserPoints[data->as.laser.laserPointCount++];
//     *pointA            = pos;

//     float  finalCharge = data->charge * HITINTERVAL;
//     float  finalDamage = data->damage * finalCharge;
//     SolRay ray1        = {
//         .pos       = pos,
//         .dir       = dir,
//         .dist      = Whip_LENGTH,
//         .ignoreEnt = id,
//     };
//     SolRayResult ray1Result = Sol_Raycast(world, ray1);
//     *pointB                 = ray1Result.pos;

//     SolRay rayDamage = {
//         .dir       = dir,
//         .dist      = ray1.dist,
//         .ignoreEnt = id,
//         .pos       = pos,
//         .mask      = 0b1,
//     };

//     SolRayResult results[256];
//     int hits = Sol_SphereCast(world, rayDamage, Sol_Math_Lerp(0.1f, 1.25f, data->charge / MAX_CHARGE), results, 256);
//     for (int i = 0; i < hits; i++)
//     {
//         SolRayResult result = results[i];

//         if (combat->hitEnts[result.entId])
//             continue;
//         combat->hitEnts[result.entId] = true;

//         Sol_Event_Add(world, (SolEvent){
//                                  .kind              = EVENTKIND_HIT,
//                                  .as.hit.damage     = finalDamage,
//                                  .as.hit.power      = finalCharge,
//                                  .as.hit.buffMask   = data->buffs,
//                                  .as.hit.effectMask = data->effects,
//                                  .as.hit.entA       = id,
//                                  .as.hit.entB       = result.entId,
//                                  .as.hit.pos        = result.pos,
//                                  .as.hit.vel        = dir,
//                                  .as.hit.fxKind     = FXKIND_Whip_HIT,
//                              });
//     }
//     if (ray1Result.hit)
//         Laser(world, id, ray1Result.pos, Sol_BounceVec(dir, ray1Result.norm));
// }

static bool Leave_State(World *world, int id, CompAbility *ability)
{
    if (!ability->stateData[ability->activeSlot].held)
        if (Sol_Ability_SetState(world, id, ABILITY_STATE_IDLE, 0, true))
            return true;
    return false;
}

void Whip_State_Update(World *world, int id, float dt)
{
    CompAbility *ability = &world->abilities[id];
    if (Leave_State(world, id, ability))
        return;
    AbilityData *data = &ability->stateData[ability->activeSlot];
    data->elapsed += dt;

    CompController *controller = &world->controllers[id];

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
        data->charge += dt * 8.0f;
        if (data->charge >= MAX_CHARGE)
            data->stage = 2;
    }
    else
        data->charge -= dt * 1.0f;

    data->charge = fminf(MAX_CHARGE, fmaxf(MIN_CHARGE, data->charge));

    data->accum += dt;
    if (data->accum < HITINTERVAL)
        return;
    data->accum = 0;

    Sol_Combat_ClearHits(world, id);
    data->as.laser.laserPointCount = 0;
}

void Whip_State_Enter(World *world, int id)
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

void Whip_State_Exit(World *world, int id)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ability->activeSlot];
    data->lastExited     = solState.gameTime;
    Sol_Model_PlayAnim(world, id, (AnimDesc){.layerId = ANIM_LAYER_UPPER});
    Sol_World_Audio_Remove(world, id, 0);
}

bool Whip_State_CanExit(World *world, int id, u32 next)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ability->activeSlot];
    return true;
}

bool Whip_State_CanEnter(World *world, int id, u32 last, u32 next, int slot)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[slot];
    return slot != ability->activeSlot && !(data->lastExited + data->cooldown > solState.gameTime);
}
