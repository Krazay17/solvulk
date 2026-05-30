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
        Sol_Ability_SetState(world, id, ABILITY_STATE_IDLE, false);
    }
}

void Claw_State_Enter(World *world, int id)
{
    Sol_Audio_PlayAt(SOL_AUDIO_FIREBALL, Sol_Controller_GetAimPos(world, id), 0.0f);

    CompXform   *xform  = &world->xforms[id];
    CompAbility *combat = &world->abilities[id];

    float velocity = 40.0f;

    AnimDesc desc = {
        .anim = ANIM_ABILITY0, .layerId = ANIM_LAYER_UPPER, .seek = 0.16f, .force = true};
    Sol_Model_PlayAnim(world, id, desc);
}

void Claw_State_Exit(World *world, int id)
{
    Sol_Model_PlayAnim(world, id, (AnimDesc){ .layerId = ANIM_LAYER_OVERRIDE});
}

bool Claw_State_CanExit(World *world, int id, u32 next)
{
    AbilityData *data = &world->abilities[id].stateData[world->abilities[id].state];
    return data->elapsed >= CLAW_DURATION * 0.6f;
}

bool Claw_State_CanEnter(World *world, int id, u32 last, u32 next)
{
    AbilityData *data = &world->abilities[id].stateData[next];
    return !(data->lastEntered + CLAW_COOLDOWN > (float)Sol_GetState()->gameTime);
}
