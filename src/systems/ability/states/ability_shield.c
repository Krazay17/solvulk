#include "world.h"
#include "estate.h"
#include "sol_math.h"
#include "render/render.h"

#define HITINTERVAL ((float)(SOL_TIMESTEP * 0.5f))

#define MINPOWER 0.2f

static float Scale(const AbilityStateData *data, float elapsed)
{
    return Sol_Math_MapRange(1.0f, 5.0f, 0, 1.0f, (elapsed / data->conf.duration) * data->power);
}

static void Fire(World *world, int id, vec3s pos, const AbilityStateData *data, float radius)
{
    Sol_Combat_DamageSphere(world, id,
                            (SolRay){
                                .radius    = radius,
                                .start     = pos,
                                .ignoreEnt = id,
                            },
                            (SolHit){
                                .damage     = data->conf.damage,
                                .entA       = id,
                                .kind       = HITKIND_NORMAL,
                                .power      = data->power,
                            },
                            data->hitgen);
}

static void Charge(World *world, int id, ScAbility *ability, ScCmd *cmd , int slot, float dt)
{
    AbilityStateData *data = &ability->stateData[slot];

    switch (data->stage)
    {
    case 0:
        if (!data->held)
        {
            data->stage++;
            goto fire;
        }
        data->power = min(data->conf.maxpower, data->power + (dt * data->conf.speed));
        ScMove3 *move3 = Sol_Comp_Get(world, id, ScMove3);
        if(move3)
            move3->speedMod = Sol_Math_Lerp(1.0f, 0.5f, data->power / data->conf.maxpower);
        break;
    case 1:
    fire:
        vec3s pos = world->xform.pos[id];
        Sol_Combat_DamageSphere(world, id,
                                (SolRay){
                                    .radius    = Scale(data, data->elapsed),
                                    .start     = pos,
                                    .ignoreEnt = id,
                                },
                                (SolHit){
                                    .damage     = data->conf.damage,
                                    .entA       = id,
                                    .kind       = HITKIND_NORMAL,
                                    .power      = data->power,
                                },
                                data->hitgen);
        Sol_Combat_DamageSphere(world, id,
                                (SolRay){
                                    .radius    = Sol_Math_Lerp(1.0f, 3.0f, data->elapsed / data->conf.duration),
                                    .start     = pos,
                                    .ignoreEnt = id,
                                },
                                (SolHit){
                                    .damage     = data->conf.damage,
                                    .entA       = id,
                                    .kind       = HITKIND_NORMAL,
                                    .power      = 1.0f,
                                },
                                data->hitgen2);
        break;
    }

    if (data->stage > 0)
    {
        data->elapsed += dt;
        if (data->elapsed >= data->conf.duration)
            Sol_Ability_SetState(world, id, 0, slot, true);
    }
}

static void Spell(World *world, int id, ScAbility *ability, ScCmd *cmd , int slot, float dt)
{
    AbilityStateData *data = &ability->stateData[slot];

    data->power = 1.0f;
    data->elapsed += dt;
    vec3s pos = world->xform.pos[id];
    Sol_Combat_DamageSphere(world, id,
                            (SolRay){
                                .radius    = Scale(data, data->elapsed),
                                .start     = pos,
                                .ignoreEnt = id,
                            },
                            (SolHit){
                                .damage     = data->conf.damage,
                                .entA       = id,
                                .kind       = HITKIND_NORMAL,
                                .power      = 1.0f,
                            },
                            data->hitgen);
    if (data->elapsed >= data->conf.duration)
    {
        Sol_Ability_SetState(world, id, 0, slot, true);
    }
}

static void Enter(World *world, int id, ScAbility *ability, ScCmd *cmd , int slot)
{
    AbilityStateData *data  = &ability->stateData[slot];
    data->conf              = Sol_Ability_GetSlotConf(ability, slot);
    data->hitgen            = Sol_Hitgen_Start(world, id);
    data->hitgen2           = Sol_Hitgen_Start(world, id);
    data->drawElapsed       = 0.0f;
    data->power             = MINPOWER;
}
static void Exit(World *world, int id, ScAbility *ability, ScCmd *cmd , int slot)
{
    AbilityStateData *data  = &ability->stateData[slot];
    data->cooldownRemaining = data->conf.cooldown;
}
static bool CanExit(World *world, int id, ScAbility *ability, ScCmd *cmd , int slot)
{
    return true;
}
static bool CanEnter(World *world, int id, ScAbility *ability, ScCmd *cmd , int slot)
{
    AbilityStateData *data = &ability->stateData[slot];
    return !(data->cooldownRemaining > 0.0f);
}
static void Draw(World *world, int id, ScAbility *ability, int slot, float dt)
{
    AbilityStateData *data = &ability->stateData[slot];
    data->drawElapsed += dt;
    SphereSSBO *ss = Sol_Render_GetNextSphere(PIPE_SPHERE_FX);
    vec4s pos      = {world->xform.draw_pos[id].x, world->xform.draw_pos[id].y, world->xform.draw_pos[id].z,
                      Scale(data, data->drawElapsed)};
    ss->pos        = pos;
    ss->color      = (vec4s){1, 0, 1, 1};
}
static void Draw_Charge(World *world, int id, ScAbility *ability, int slot, float dt)
{
    AbilityStateData *data = &ability->stateData[slot];
    switch (data->stage)
    {
    case 0:
        SphereSSBO *ss = Sol_Render_GetNextSphere(PIPE_SPHERE_FX);
        ss->pos =
            (vec4s){world->xform.draw_pos[id].x, world->xform.draw_pos[id].y, world->xform.draw_pos[id].z, data->power};
        ss->color = (vec4s){1, 0, 1, 1};
        break;
    }
    if (data->stage > 0)
    {
        data->drawElapsed += dt;
        SphereSSBO *ss = Sol_Render_GetNextSphere(PIPE_SPHERE_FX);
        ss->pos        = (vec4s){world->xform.draw_pos[id].x, world->xform.draw_pos[id].y, world->xform.draw_pos[id].z,
                                 Scale(data, data->drawElapsed)};
        ss->color      = (vec4s){1, 0, 1, 1};

        SphereSSBO *s = Sol_Render_GetNextSphere(PIPE_SPHERE_FX);
        s->pos        = (vec4s){world->xform.draw_pos[id].x, world->xform.draw_pos[id].y, world->xform.draw_pos[id].z,
                                Sol_Math_Lerp(0.2f, 3.0f, data->drawElapsed / data->conf.duration)};
        s->color      = (vec4s){1, 0, 1, 1};
    }
}

const AbilityStateFunc ability_shield_state = {
    .update   = Spell,
    .enter    = Enter,
    .exit     = Exit,
    .canExit  = CanExit,
    .canEnter = CanEnter,
    .draw     = Draw,
};

const AbilityStateFunc ability_shield_charge_state = {
    .update   = Charge,
    .enter    = Enter,
    .exit     = Exit,
    .canExit  = CanExit,
    .canEnter = CanEnter,
    .draw     = Draw_Charge,
};