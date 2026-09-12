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

extern const AbilityStateFunc idle_state;
extern const AbilityStateFunc claw_state;
extern const AbilityStateFunc fireball_state;
extern const AbilityStateFunc dash_state;

const AbilityStateFunc *ability_state_func[ABILITY_STATE_COUNT] = {
    [ABILITY_STATE_IDLE]     = &idle_state,
    [ABILITY_STATE_CLAW]     = &claw_state,
    [ABILITY_STATE_FIREBALL] = &fireball_state,
    [ABILITY_STATE_DASH]     = &dash_state,
};

void Ability_Step(World *world, double dt)
{
    float fdt = (float)dt;

    SparseSet_ScAbility *set = Sol_Comp_Set(world, ScAbility);
    for (int i = 0; i < set->cnt; i++)
    {
        int id             = set->dense[i];
        ScAbility *ability = &set->data[i];
        ScCmd *cmd         = Sol_Comp_Get(world, id, ScCmd);
        if (!cmd)
            continue;
        for (int j = 0; j < ability->slots; j++)
        {
            AbilityStateData *data     = &ability->stateData[j];
            data->cooldownRemaining    = fmaxf(0.0f, data->cooldownRemaining - fdt);
            int mask                   = BITC(ACTION_ABILITY1 + j);
            bool held                  = cmd->actionState & mask;
            ability->stateData[j].held = held;
            if (held && ability->activeSlot != j)
            {
                Sol_Ability_SetState(world, id, ability->action_map[j], j, false);
            }
        }
        
        const AbilityStateFunc *state_func = ability_state_func[ability->state];
        if (state_func && state_func->update)
            state_func->update(world, id, ability, cmd, fdt);
    }
}

void Ability_Draw(World *world, double dt)
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
