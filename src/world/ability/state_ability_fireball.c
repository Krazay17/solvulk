#include "ability_i.h"
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
#define MAX_POWER 2.5f
#define MAX_DURATION 4.0f

#define RECOVERYTIME 0.3f
#define MIN_VELOCITY 20.0f
#define MAX_VELOCITY 45.0f

void Fireball_State_Update(World *world, int id, float dt)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ability->activeSlot];
    float       *elapsed = &data->elapsed;

    float oldElapsed = data->elapsed;
    data->elapsed += dt;

    data->charge = fminf(1.0f, data->elapsed / MAX_DURATION);

    float power = Sol_Math_Lerp(MIN_POWER, MAX_POWER, data->charge);

    switch (data->stage)
    {
    case 0:
        if (!data->held)
            data->stage++;
        break;
    case 1:
        AnimDesc desc = {
            .anim = ANIM_ATTACK_LEFT, .layerId = ANIM_LAYER_UPPER, .seek = 0.16f, .playKind = ANIMPLAYKIND_ONESHOT};
        Sol_Model_PlayAnim(world, id, desc);

        SolShoot shoot = Sol_Controller_GetShoot(world, id, Sol_Math_Lerp(MAX_VELOCITY, MIN_VELOCITY, data->charge));
        int ball = Sol_Prefab_Factory(world, 0, EKIND_FIREBALL,
                                      (EntDesc){.pos = vecAdd(shoot.pos, vecSca(WORLD_UP, power)), .scale = power});

        if (ball > 0)
        {
            Sol_Physx_SetVel(world, ball, shoot.vel);
            Sol_Owner_Add(world, ball, id);
            world->projectiles[ball].directHit.damage        = data->damage;
            world->projectiles[ball].explosionHit.damage     = data->damage * power;
            world->projectiles[ball].explosionHit.buffMask   = data->buffs;
            world->projectiles[ball].explosionHit.effectMask = data->effects;
            world->projectiles[ball].explodeRadius           = 2.0f * power;
        }

        data->stage++;
        break;
    case 2:
        data->recovery += dt;
        if (data->recovery >= RECOVERYTIME)
            Sol_Ability_SetState(world, id, ABILITY_STATE_IDLE, 0, false);
    }
}

void Fireball_State_Enter(World *world, int id)
{
    CompXform   *xform  = &world->xforms[id];
    CompAbility *combat = &world->abilities[id];
    AbilityData *data   = &world->abilities[id].stateData[combat->activeSlot];
    data->stage         = 0;
    data->recovery      = 0.0f;

    AnimDesc desc = {.anim = ANIM_CHARGE_LEFT, .layerId = ANIM_LAYER_UPPER};
    Sol_Model_PlayAnim(world, id, desc);
}

void Fireball_State_Exit(World *world, int id)
{
    CompAbility *a    = &world->abilities[id];
    AbilityData *data = &world->abilities[id].stateData[a->activeSlot];
    data->lastExited  = solState.gameTime;
    data->charge      = 0;
    data->stage       = 0;
    Sol_Model_PlayAnim(world, id, (AnimDesc){.layerId = ANIM_LAYER_UPPER});
}

bool Fireball_State_CanExit(World *world, int id, u32 next)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &world->abilities[id].stateData[ability->activeSlot];
    return data->stage > 1;
}

bool Fireball_State_CanEnter(World *world, int id, u32 last, u32 next, int slot)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &world->abilities[id].stateData[slot];
    return slot != ability->activeSlot && !(data->lastExited + data->cooldown > solState.gameTime);
}

void Fireball_State_Draw(World *world, int id, double dt, double time)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &world->abilities[id].stateData[ability->activeSlot];

    if (data->stage > 0)
        return;
    float scale = data->charge * 2.0f + 0.5f;
    vec3s pos   = Sol_Model_GetBoneXform(world, id, "hand.L").pos;
    pos         = vecAdd(pos, vecSca(Sol_Controller_GetAimdir(world, id), scale));
    pos         = vecAdd(pos, vecSca(WORLD_UP, scale));

    SphereSSBO *push = Sol_Render_GetNext_Fireball();
    push->pos        = (vec4s){pos.x, pos.y, pos.z, scale};
    push->color      = (vec4s){1, 0, 0, 0.8f};
}