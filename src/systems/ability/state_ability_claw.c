#include "sol_core.h"

#include "ability_i.h"

#define CLAW_DURATION 0.6f
#define CLAW_COOLDOWN 0.6f

void Claw_State_Update(World *world, int id, float dt)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ability->state];
    float       *elapsed = &data->elapsed;
    *elapsed += dt;
    if (*elapsed >= CLAW_DURATION)
    {
        Sol_Ability_SetState(world, id, ABILITY_STATE_IDLE);
    }
}

void Claw_State_Enter(World *world, int id)
{
    Sol_Audio_Play(SOL_AUDIO_FIREBALL);

    CompXform   *xform  = &world->xforms[id];
    CompAbility *combat = &world->abilities[id];

    float velocity = 40.0f;

    int ball = Sol_Prefab_Fireball(world, Sol_Controller_GetShootpos(world, id, 0.4f),
                                   vecSca(Sol_Controller_GetAimdir(world, id), velocity), id);

    AnimDesc desc = {
        .anim = ANIM_ABILITY0, .blendIn = 15.0f, .layerId = ANIM_LAYER_UPPER, .seek = 0.16f, .force = true};
    Sol_Model_PlayAnim(world, id, desc);
}

void Claw_State_Exit(World *world, int id)
{
    Sol_Model_StopAnim(world, id, ANIM_LAYER_UPPER, 0.1f);
}

bool Claw_State_CanEnter(World *world, int id, int last)
{
    AbilityData *data = &world->abilities[id].stateData[ABILITY_STATE_CLAW];
    return !(data->lastEntered + CLAW_COOLDOWN > (float)Sol_GetState()->gameTime);
}

bool Claw_State_CanExit(World *world, int id, int next)
{
    AbilityData *data = &world->abilities[id].stateData[world->abilities[id].state];
    return data->elapsed >= CLAW_DURATION * 0.6f;
}
