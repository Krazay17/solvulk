#include "ability/si_ability.h"
#include "sol_core.h"
#include "world.h"
#include "sol_math.h"
#include "audio.h"
#include "xform/s_xform.h"
#include "controller/s_controller.h"
#include "combat/s_combat.h"
#include "model/s_model.h"
#include "event/s_event.h"
#include "physx/s_body.h"
#include "movement/s_movement.h"
#include "buff/s_buff.h"
#include "projectile/s_projectile.h"
#include "owner/s_owner.h"
#include "game/prefabs.h"
#include "render/render.h"

#define MIN_POWER 0.5f

#define RECOVERYTIME 0.3f
#define MIN_VELOCITY 20.0f
#define MAX_VELOCITY 45.0f

void Fireball_Linger(int flags, void *data)
{
    HitCallbackData *callbackData = data;
    int id = Sol_Prefab_HealZone(callbackData->world, callbackData->pos, callbackData->scale, callbackData->instigator);
}

void Fireball_State_Update(World *world, int id, float dt)
{
    CompAbility      *ability    = Sol_Ability_Get(world, id);
    AbilityStateData *data       = &ability->stateData[ability->activeSlot];
    float            *elapsed    = &data->elapsed;
    float             oldElapsed = data->elapsed;
    data->elapsed += dt;

    data->power = fminf(1.0f, data->elapsed / ability_base[ABILITY_STATE_FIREBALL].duration);
    sollog(data->power);
    float power = Sol_Math_Lerp(MIN_POWER, ability_base[ABILITY_STATE_FIREBALL].maxpower, data->power);

    switch (data->stage)
    {
    case 0:
        if (!data->held)
            data->stage++;
        break;
    case 1:
        SolShoot shoot =
            Sol_Controller_GetShoot(world, id, 1.5f, Sol_Math_Lerp(MAX_VELOCITY, MIN_VELOCITY, data->power));
        int ball = Sol_Prefab_Factory(world, 0, EKIND_FIREBALL,
                                      (EntDesc){.pos = vecAdd(shoot.pos, vecSca(WORLD_UP, power)), .scale = power});
        if (ball > 0)
        {
            Sol_Physx_SetVel(world, ball, shoot.vel);
            Sol_Owner_Add(world, ball, id);
            world->projectiles[ball].directHit.damage        = ability_base[ABILITY_STATE_FIREBALL].damage;
            world->projectiles[ball].explosionHit.damage     = ability_base[ABILITY_STATE_FIREBALL].damage * power;
            world->projectiles[ball].explosionHit.buffMask   = ability_base[ABILITY_STATE_FIREBALL].buffMask;
            world->projectiles[ball].explosionHit.effectMask = ability_base[ABILITY_STATE_FIREBALL].effectMask;
            world->projectiles[ball].explodeRadius           = 2.0f * power;
            world->projectiles[ball].callback.callbackFunc   = Fireball_Linger;
            world->projectiles[ball].callbackFlags           = 1;
        }

        data->stage++;
        break;
    case 2:
        data->recover += dt;
        if (data->recover >= RECOVERYTIME)
            Sol_Ability_SetState(world, id, ABILITY_STATE_IDLE, 0, false);
        break;
    }
}

void Fireball_State_Enter(World *world, int id)
{
    CompXform        *xform   = &world->xforms[id];
    CompAbility      *ability = Sol_Ability_Get(world, id);
    AbilityStateData *data    = &ability->stateData[ability->activeSlot];
    data->stage               = 0;
    data->recover             = 0.0f;
}

void Fireball_State_Exit(World *world, int id)
{
    CompAbility      *ability = Sol_Ability_Get(world, id);
    AbilityStateData *data    = &ability->stateData[ability->activeSlot];
    data->lastExited          = solState.gameTime;
    data->power               = 0;
    data->stage               = 0;
    Sol_Model_StopAnim(world, id, ANIM_LAYER_UPPER, 0);
}

bool Fireball_State_CanExit(World *world, int id, u32 next)
{
    CompAbility      *ability = Sol_Ability_Get(world, id);
    AbilityStateData *data    = &ability->stateData[ability->activeSlot];
    return data->stage > 1;
}

bool Fireball_State_CanEnter(World *world, int id, u32 last, u32 next, int slot)
{
    CompAbility      *ability = Sol_Ability_Get(world, id);
    AbilityStateData *data    = &ability->stateData[slot];
    return slot != ability->activeSlot &&
           !(data->lastExited + ability_base[ABILITY_STATE_FIREBALL].cooldown > solState.gameTime);
}

void Fireball_State_Draw(World *world, int id, double dt, double time)
{
    CompAbility      *ability = Sol_Ability_Get(world, id);
    AbilityStateData *data    = &ability->stateData[ability->activeSlot];

    if (data->stage > 0)
        return;
    float scale = data->power * 2.0f + 0.5f;
    vec3s pos   = Sol_Model_GetBoneXform(world, id, "hand.L").pos;
    pos         = vecAdd(pos, vecSca(Sol_Controller_Get(world, id)->aimdir, scale));
    pos         = vecAdd(pos, vecSca(WORLD_UP, scale));

    SphereSSBO *push = Sol_Render_GetNext_Fireball();
    push->pos        = (vec4s){pos.x, pos.y, pos.z, scale};
    push->color      = (vec4s){1, 0, 0, 0.8f};
}