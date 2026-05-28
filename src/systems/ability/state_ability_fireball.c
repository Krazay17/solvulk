#include "sol_core.h"

#include "ability_i.h"

#define MIN_POWER 0.5f
#define MAX_POWER 3.0f
#define MAX_DURATION 4.0f

#define RECOVERYTIME 0.3f
#define COOLDOWN 0.2f
#define BALL_VELOCITY 40.0f

void Fireball_State_Update(World *world, int id, float dt)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ability->state];
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
            .anim = ANIM_ATTACK_LEFT, .blendIn = 5.0f, .layerId = ANIM_LAYER_UPPER, .seek = 0.16f, .force = true};
        Sol_Model_PlayAnim(world, id, desc);
        Sol_Audio_PlayAt(SOL_AUDIO_FIREBALL, Sol_Controller_GetAimPos(world, id), 0.0f);

        int ball =
            Sol_Prefab_Factory(world, 0, PREFABKIND_FIREBALL,
                               (PrefabDesc){.pos = Sol_Controller_GetShootPos(world, id, 0.33f), .scale = power});
        if (ball > 0)
        {
            Sol_Physx_SetVel(world, ball, vecSca(Sol_Controller_GetAimdir(world, id), BALL_VELOCITY));
            Sol_Owner_SetOwner(world, ball, id);
        }

        data->stage++;
        break;
    case 2:
        data->recovery += dt;
        if (data->recovery >= RECOVERYTIME)
            Sol_Ability_SetState(world, id, ABILITY_STATE_IDLE, false);
    }
}

void Fireball_State_Enter(World *world, int id)
{
    CompXform   *xform  = &world->xforms[id];
    CompAbility *combat = &world->abilities[id];
    AbilityData *data   = &world->abilities[id].stateData[ABILITY_STATE_FIREBALL];
    data->stage         = 0;
    data->recovery      = 0.0f;

    AnimDesc desc = {
        .anim = ANIM_CHARGE_LEFT, .blendIn = 15.0f, .layerId = ANIM_LAYER_UPPER, .seek = 0.16f, .force = true};
    Sol_Model_PlayAnim(world, id, desc);
}

void Fireball_State_Exit(World *world, int id)
{
    Sol_Model_PlayAnim(world, id, (AnimDesc){.anim = 0, .layerId = ANIM_LAYER_UPPER, .blendOut = 0.2f});
}

bool Fireball_State_CanExit(World *world, int id, u32 next)
{
    AbilityData *data = &world->abilities[id].stateData[ABILITY_STATE_FIREBALL];
    return next != ABILITY_STATE_FIREBALL;
}

bool Fireball_State_CanEnter(World *world, int id, u32 last, u32 next)
{
    AbilityData *data = &world->abilities[id].stateData[next];
    return !(data->lastEntered + COOLDOWN > Sol_GetGameTime());
}
