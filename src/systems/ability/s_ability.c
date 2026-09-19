/*
 * File: s_ability.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-04
 *
 */
#include "world.h"
#include "estate.h"
#include "sol_core.h"

const char *ability_state_name[ABILITY_STATE_COUNT] = {
    [ABILITY_STATE_IDLE] = "Idle",           [ABILITY_STATE_DASH] = "Dash",
    [ABILITY_STATE_FIREBALL] = "Fireball",   [ABILITY_STATE_PISTOL] = "Pistol",
    [ABILITY_STATE_SPINSLASH] = "Spinslash", [ABILITY_STATE_CLAW] = "Claw",
    [ABILITY_STATE_SHIELD] = "Shield",       [ABILITY_STATE_LASER] = "Laser",
    [ABILITY_STATE_WHIP] = "Whip",           [ABILITY_STATE_FIREBALLVOLLEY] = "FireballVolley",
};

const u32 ability_texture_map[ABILITY_STATE_COUNT] = {
    [ABILITY_STATE_IDLE]     = 0,
    [ABILITY_STATE_DASH]     = SOL_TEXTURE_DASH_CARD,
    [ABILITY_STATE_CLAW]     = SOL_TEXTURE_BLADE_CARD,
    [ABILITY_STATE_FIREBALL] = SOL_TEXTURE_FIREBALL_CARD,
    [ABILITY_STATE_PISTOL]   = SOL_TEXTURE_PISTOL_CARD,
    // [ABILITY_STATE_SPINSLASH] = ,
    // [ABILITY_STATE_SHIELD] = ,
    // [ABILITY_STATE_LASER] = ,
    // [ABILITY_STATE_WHIP] = ,
    // [ABILITY_STATE_FIREBALLVOLLEY] = ,
};

const AbilityConfig ability_base[ABILITY_STATE_COUNT] = {
    [ABILITY_STATE_IDLE] =
        {
            0,
        },
    [ABILITY_STATE_DASH] =
        {
            .duration   = 0.3f,
            .cooldown   = 1.0f,
            .damage     = 10.0f,
            .effectMask = EFFECTMASK_KNOCKBACK,
            .buffMask   = BUFFKIND_FIRE,
            .maxpower   = 4.0f,
        },
    [ABILITY_STATE_FIREBALL] =
        {
            .duration        = 3.0f,
            .recoverDuration = 0.5f,
            .cooldown        = 1.0f,
            .damage          = 10.0f,
            .effectMask      = EFFECTMASK_KNOCKBACK,
            .buffMask        = BUFFKIND_FIRE,
            .maxpower        = 2.0f,
        },
    [ABILITY_STATE_PISTOL] =
        {
            .duration   = 0.5f,
            .cooldown   = 0.0f,
            .damage     = 10.0f,
            .effectMask = EFFECTMASK_KNOCKBACK,
            .buffMask   = BUFFKIND_FIRE,
            .maxpower   = 4.0f,
        },
    [ABILITY_STATE_SPINSLASH] =
        {
            .duration   = 0.5f,
            .cooldown   = 0.0f,
            .damage     = 10.0f,
            .effectMask = EFFECTMASK_KNOCKBACK,
            .buffMask   = BUFFKIND_FIRE,
            .maxpower   = 4.0f,
        },
    [ABILITY_STATE_CLAW] =
        {
            .duration   = 0.5f,
            .cooldown   = 1.0f,
            .damage     = 10.0f,
            .effectMask = EFFECTMASK_KNOCKBACK,
            .buffMask   = BUFFKIND_FIRE,
            .maxpower   = 4.0f,
        },
    [ABILITY_STATE_SHIELD] =
        {
            .duration   = 0.5f,
            .cooldown   = 0.0f,
            .damage     = 10.0f,
            .effectMask = EFFECTMASK_KNOCKBACK,
            .buffMask   = BUFFKIND_FIRE,
            .maxpower   = 4.0f,
        },
    [ABILITY_STATE_LASER] =
        {
            .duration   = 0.5f,
            .cooldown   = 0.0f,
            .damage     = 10.0f,
            .effectMask = EFFECTMASK_KNOCKBACK,
            .buffMask   = BUFFKIND_FIRE,
            .maxpower   = 4.0f,
        },
    [ABILITY_STATE_WHIP] =
        {
            .duration   = 0.5f,
            .cooldown   = 0.0f,
            .damage     = 10.0f,
            .effectMask = EFFECTMASK_KNOCKBACK,
            .buffMask   = BUFFKIND_FIRE,
            .maxpower   = 4.0f,
        },
    [ABILITY_STATE_FIREBALLVOLLEY] =
        {
            .duration   = 0.5f,
            .cooldown   = 0.0f,
            .damage     = 10.0f,
            .effectMask = EFFECTMASK_KNOCKBACK,
            .buffMask   = BUFFKIND_FIRE,
            .maxpower   = 4.0f,
        },
};


extern const AbilityStateFunc ability_idle_state;
extern const AbilityStateFunc ability_claw_state;
extern const AbilityStateFunc ability_fireball_state;
extern const AbilityStateFunc ability_dash_state;

const AbilityStateFunc *ability_state_func[ABILITY_STATE_COUNT] = {
    [ABILITY_STATE_IDLE]     = &ability_idle_state,
    [ABILITY_STATE_CLAW]     = &ability_claw_state,
    [ABILITY_STATE_FIREBALL] = &ability_fireball_state,
    [ABILITY_STATE_DASH]     = &ability_dash_state,
};

void Ability_Step(World *world)
{
    float fdt = world->timestep;

    SparseSet_ScAbility *set = Sol_Comp_Set(world, ScAbility);
    for (int i = 0; i < set->cnt; i++)
    {
        int id             = set->dense[i];
        ScAbility *ability = &set->data[i];
        ScCmd *cmd         = Sol_Comp_Get(world, id, ScCmd);
        if (!cmd)
            continue;
        bool is_idle = (ability->state == ABILITY_STATE_IDLE);
        for (int j = 0; j < ABILITY_SLOTS; j++)
        {
            AbilityStateData *data     = &ability->stateData[j];
            data->cooldownRemaining    = fmaxf(0.0f, data->cooldownRemaining - fdt);
            int mask                   = BITC(j);
            bool held                  = cmd->actionState & mask;
            ability->stateData[j].held = held;
            if (held && (is_idle || ability->activeSlot != j))
            {
                Sol_Ability_SetState(world, id, ability->action_map[j], j, false);
                break;
            }
        }

        const AbilityStateFunc *state_func = ability_state_func[ability->state];
        if (state_func && state_func->update)
            state_func->update(world, id, ability, cmd, fdt);
    }
}

void Ability_Draw(World *world)
{
    SparseSet_ScAbility *set = Sol_Comp_Set(world, ScAbility);
    for (int i = 0; i < set->cnt; i++)
    {
        int id             = set->dense[i];
        ScAbility *ability = &set->data[i];

        if (!Sol_Comp_Has(world, id, ScCmd))
            continue;
        ScCmd *cmd = Sol_Comp_Get(world, id, ScCmd);

        const AbilityStateFunc *state_func = ability_state_func[ability->state];
        if (state_func && state_func->draw)
            state_func->draw(world, id, ability, cmd);
    }
}

bool Sol_Ability_SetState(World *world, int id, AbilityState target_state, int slot, bool force)
{
    if (target_state >= ABILITY_STATE_COUNT)
        return false;
    ScAbility *ability               = Sol_Comp_Get(world, id, ScAbility);
    ScCmd *cmd                       = Sol_Comp_Get(world, id, ScCmd);
    const AbilityStateFunc *prevfunc = ability_state_func[ability->state];
    const AbilityStateFunc *nextfunc = ability_state_func[target_state];
    if (!prevfunc || !nextfunc)
        return false;

    if (!force)
    {
        if (!prevfunc->canExit || !prevfunc->canExit(world, id, ability, cmd, target_state))
            return false;
        if (!nextfunc->canEnter || !nextfunc->canEnter(world, id, ability, cmd, ability->state, slot))
            return false;
    }

    if (prevfunc->exit)
        prevfunc->exit(world, id, ability, cmd);

    ability->state      = target_state;
    ability->activeSlot = slot;

    AbilityStateData *data = &ability->stateData[slot];
    data->elapsed          = 0;
    data->accum            = 0;
    data->stage            = 0;
    data->recover          = 0;
    data->power            = 0;

    if (nextfunc->enter)
        nextfunc->enter(world, id, ability, cmd);

    return true;
}
