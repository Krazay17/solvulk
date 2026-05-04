#include "ability_states.h"
#include "sol_core.h"

#define CLAW_DURATION 1.0f
#define CLAW_COOLDOWN 1.0f

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
    CompController *controller = &world->controllers[id];
    CompXform      *xform      = &world->xforms[id];
    CompModel      *model      = &world->models[id];
    CompAbility    *combat     = &world->abilities[id];

    vec3s aimpos = controller->aimpos;
    vec3s aimdir = controller->aimdir;

    float min      = 0.2f;
    float max      = 0.8f;
    float randSize = min + (float)rand() / (float)RAND_MAX * (max - min);

    int ball = Sol_Prefab_Ball(
        world, vecAdd(aimpos, vecSca(aimdir, 5.0f)), vecSca(aimdir, 25.0f),
        (CompSphere){.radius = randSize, .color = (vec4s){rand() % 255, rand() % 255, rand() % 255, 255}});

    Sol_Model_PlayAnim(world, id, ANIM_ABILITY0, 6.0f);
}
void Claw_State_Exit(World *world, int id)
{
}
bool Claw_State_CanEnter(World *world, int id)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ABILITY_STATE_CLAW];
    if (data->lastEntered + CLAW_COOLDOWN > (float)Sol_GetState()->gameTime)
        return false;

    return true;
}
bool Claw_State_CanExit(World *world, int id)
{
    return true;
}
