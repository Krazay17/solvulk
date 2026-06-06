#include "sol_core.h"

#include "ability_i.h"

#define MIN_POWER 0.5f
#define MAX_POWER 3.0f
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
        AnimDesc desc = {.anim = ANIM_ATTACK_LEFT, .layerId = ANIM_LAYER_UPPER, .seek = 0.16f, .oneShot = true};
        Sol_Model_PlayAnim(world, id, desc);

        SolShoot shoot = Sol_Controller_GetShoot(world, id, Sol_Math_Lerp(MAX_VELOCITY, MIN_VELOCITY, data->charge));
        int ball = Sol_Prefab_Factory(world, 0, ENTKIND_FIREBALL,
                                      (EntDesc){.pos = vecAdd(shoot.pos, vecSca(WORLD_UP, power)), .scale = power});

        if (ball > 0)
        {
            Sol_Physx_SetVel(world, ball, shoot.vel);
            Sol_Owner_Add(world, ball, id);
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
    data->lastExited  = Sol_GetGameTime();
    Sol_Model_PlayAnim(world, id, (AnimDesc){.layerId = ANIM_LAYER_UPPER});
}

bool Fireball_State_CanExit(World *world, int id, u32 next)
{
    return true; //next != ABILITY_STATE_FIREBALL;
}

bool Fireball_State_CanEnter(World *world, int id, u32 last, u32 next, u32 slot)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &world->abilities[id].stateData[slot];
    if (slot == ability->activeSlot)
        return false;

    return !(data->lastExited + ability_config[ABILITY_STATE_FIREBALL].cooldown > Sol_GetGameTime());
}
