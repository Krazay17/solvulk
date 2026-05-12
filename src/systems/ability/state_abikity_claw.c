#include "sol_core.h"

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
    Sol_PlayAudio(SOL_AUDIO_SPACEGUN);

    CompXform      *xform      = &world->xforms[id];
    CompAbility    *combat     = &world->abilities[id];

    vec3s aimpos = Sol_GetAimpos(world, id);
    vec3s aimdir = Sol_GetAimdir(world, id);

    float min      = 0.2f;
    float max      = 0.8f;
    float randSize = min + (float)rand() / (float)RAND_MAX * (max - min);

    int ball = Sol_Prefab_Ball(
        world, vecAdd(aimpos, vecSca(aimdir, 1.0f)), vecSca(aimdir, 35.0f),
        (ShapeDesc){.radius = randSize, .color = (vec4s){rand() % 255, rand() % 255, rand() % 255, 255}});

    AnimDesc desc = {
        .anim = ANIM_ABILITY0, .blendIn = 15.0f, .layerId = ANIM_LAYER_UPPER, .seek = 0.16f, .force = true};
    Sol_Model_PlayAnim(world, id, desc);
}

void Claw_State_Exit(World *world, int id)
{
    Sol_Model_StopAnim(world, id, ANIM_LAYER_UPPER, 0.1);
}

bool Claw_State_CanEnter(World *world, int id, int last)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ABILITY_STATE_CLAW];
    if (data->lastEntered + CLAW_COOLDOWN > (float)Sol_GetState()->gameTime)
        return false;

    return true;
}
bool Claw_State_CanExit(World *world, int id, int next)
{
    return true;
}
