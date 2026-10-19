#include "ability/s_ability.h"
#include "world.h"
#include "render/render.h"
#include "ribbon/s_ribbon.h"

static void Update(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot, float dt)
{
    AbilityStateData *data = &ability->stateData[slot];
    if (!data->held)
        Sol_Ability_SetState(world, id, 0, slot, true);
    forc(world, id, ScMove3) c->speedMod = 0.5f;
    data->as.heal.accum += dt;
    if (data->as.heal.accum >= data->as.heal.rate)
    {
        data->as.heal.accum -= data->as.heal.rate;
        Sol_Combat_Heal(world, id, id, data->as.heal.amount);
    }
}
static void Enter(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot)
{
    AbilityStateData *data = &ability->stateData[slot];
    data->as.heal.accum    = 0;
    data->as.heal.amount   = 2.0f;
    data->as.heal.rate     = 0.2f;
}
static void Exit(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot)
{
}
static bool CanExit(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot)
{
    return true;
}
static bool CanEnter(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot)
{
    return true;
}
static void Draw(World *world, int id, ScAbility *ability, int slot, float dt)
{
    vec3s posL = Sol_Model_GetBoneXform(world, id, "weapon.L").pos;
    vec3s posR = Sol_Model_GetBoneXform(world, id, "weapon.R").pos;
    vec3s top  = vecAdd(world->xform.pos[id], vecSca(WORLD_UP, 25.0f));

    Ribbon r = {
        .points[0]            = posL,
        .points[1]            = top,
        .thickness[0]         = 0.1f,
        .thickness[1]         = 0.3f,
        .nonuniform_thickness = true,
        .texture              = SOL_TEXTURE_BEAM,
        .color                = VEC4_WHITE,
        .pan                  = 4.0f,
        .sheets               = 2,
        ._point_count         = 2,
    };

    ribbon_draw_kind[RIBBONKIND_BASIC](world, dt, &r);
    r.points[0] = posR;
    ribbon_draw_kind[RIBBONKIND_BASIC](world, dt, &r);
}
static DefendResult Defend(World *world, int id, ScAbility *ability, SolHit *hit, int slot)
{
    return DEFENDKIND_NONE;
}

const AbilityStateFunc ability_heal_state = {Update, Enter, Exit, CanExit, CanEnter, Draw, Defend};