#include "world.h"
#include "estate.h"
#include "sol_math.h"
#include "render/render.h"

#define HITINTERVAL 0.1f

float Scale(const AbilityStateData *data, float delta)
{
    return Sol_Math_MapRange(1.0f, 4.0f, 0, 1.0f, delta);
}

static void Spell(World *world, int id, ScAbility *ability, ScCmd *cmd, float dt)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];
    data->elapsed += dt;
    data->accum += dt;

    vec3s pos   = world->xform.pos[id];
    data->power = data->elapsed / data->conf.duration;

    if (data->accum >= HITINTERVAL)
    {
        data->accum -= HITINTERVAL;

        Sol_Combat_DamageSphere(world, id,
                                (SolRay){
                                    .radius    = Scale(data, data->power),
                                    .start     = pos,
                                    .ignoreEnt = id,
                                },
                                (SolHit){
                                    .damage     = data->conf.damage,
                                    .buffMask   = data->conf.buffMask,
                                    .effectMask = data->conf.effectMask,
                                    .entA       = id,
                                    .kind       = HITKIND_NORMAL,
                                    .power      = 1.0f,
                                },
                                data->hitgen);
    }
    if (data->elapsed >= data->conf.duration)
    {
        Sol_Ability_SetState(world, id, 0, ability->activeSlot, true);
    }
}

static void Enter(World *world, int id, ScAbility *ability, ScCmd *cmd)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];
    data->conf             = Sol_Ability_GetSlotConf(ability, ability->activeSlot);
    data->hitgen           = Sol_Hitgen_Start(world, id);
    data->drawElapsed      = 0.0f;
    data->cooldownRemaining = data->conf.cooldown;
}
static void Exit(World *world, int id, ScAbility *ability, ScCmd *cmd)
{
}
static bool CanExit(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 next)
{
    return true;
}
static bool CanEnter(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 last, int slot)
{
    AbilityStateData *data = &ability->stateData[slot];
    return !(data->cooldownRemaining > 0.0f);
}
static void Draw(World *world, int id, ScAbility *ability, float dt)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];
    data->drawElapsed += dt;
    SphereSSBO *ss = Sol_Render_GetNextSphere(PIPE_SPHERE_FX);
    vec4s pos      = {world->xform.draw_pos[id].x, world->xform.draw_pos[id].y, world->xform.draw_pos[id].z,
                      Scale(data, data->drawElapsed / data->conf.duration)};
    ss->pos        = pos;
    ss->color      = (vec4s){1, 0, 1, 1};
}

extern const AbilityStateFunc ability_shield_state = {
    .update   = Spell,
    .enter    = Enter,
    .exit     = Exit,
    .canExit  = CanExit,
    .canEnter = CanEnter,
    .draw     = Draw,
};