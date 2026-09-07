/*
 * File: s_ability.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-04
 *
 */
#include "s_ability.h"
#include "world.h"

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
            int mask                   = BITC(ACTION_ABILITY1 + j);
            bool held                  = cmd->actionState & mask;
            ability->stateData[j].held = held;
            if (held)
            {
                Sol_Ability_SetState(world, id, ability->action_map[j], j, false);
            }
        }

        if (ABILITY_STATE_FUNC[ability->state].update)
            ABILITY_STATE_FUNC[ability->state].update(world, id, ability, cmd, fdt);
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

        if (ABILITY_STATE_FUNC[ability->state].draw)
            ABILITY_STATE_FUNC[ability->state].draw(world, id, ability, cmd);
    }
}

// ###########################
// #### PUBLIC ###############
// ###########################

bool Sol_Ability_SetState(World *world, int id, AbilityState target_state, int slot, bool force)
{
    if (target_state > ABILITY_STATE_COUNT)
        return false;
    ScAbility *ability               = Sol_Comp_Get(world, id, ScAbility);
    ScCmd *cmd                       = Sol_Comp_Get(world, id, ScCmd);
    const AbilityStateFunc *prevfunc = &ABILITY_STATE_FUNC[ability->state];
    const AbilityStateFunc *nextfunc = &ABILITY_STATE_FUNC[target_state];

    if (!force)
    {
        if (!prevfunc->canExit || !prevfunc->canExit(world, id, ability, cmd, target_state))
            return false;
        if (!nextfunc->canEnter || !nextfunc->canEnter(world, id, ability, cmd, ability->state, slot))
            return false;
    }
    if (ability->activeSlot >= 0)
        ability->stateData[ability->activeSlot].lastExited = world->tickTime;
    prevfunc->exit(world, id, ability, cmd);

    ability->state = target_state;

    ability->activeSlot                  = slot;
    ability->stateData[slot].elapsed     = 0;
    ability->stateData[slot].accum       = 0;
    ability->stateData[slot].lastEntered = world->tickTime;
    nextfunc->enter(world, id, ability, cmd);

    return true;
}