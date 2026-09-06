/*
 * File: s_ability.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-04
 *
 */
#include "s_ability.h"
#include "world.h"

const char *ability_names[ABILITY_STATE_COUNT] = {
    [ABILITY_STATE_IDLE] = "Idle",           [ABILITY_STATE_DASH] = "Dash",
    [ABILITY_STATE_FIREBALL] = "Fireball",   [ABILITY_STATE_PISTOL] = "Pistol",
    [ABILITY_STATE_SPINSLASH] = "Spinslash", [ABILITY_STATE_CLAW] = "Claw",
    [ABILITY_STATE_SHIELD] = "Shield",       [ABILITY_STATE_LASER] = "Laser",
    [ABILITY_STATE_WHIP] = "Whip",           [ABILITY_STATE_FIREBALLVOLLEY] = "FireballVolley",
};

const AbilityConfig ability_base[ABILITY_STATE_COUNT] = {
    [ABILITY_STATE_IDLE] =
        {
            0,
        },
    [ABILITY_STATE_DASH] =
        {
            .duration   = 0.3f,
            .cooldown   = 0.0f,
            .damage     = 10.0f,
            .effectMask = EFFECTMASK_KNOCKBACK,
            .buffMask   = BUFFKIND_FIRE,
            .maxpower   = 4.0f,
        },
    [ABILITY_STATE_FIREBALL] =
        {
            .duration   = 3.0f,
            .cooldown   = 0.0f,
            .damage     = 10.0f,
            .effectMask = EFFECTMASK_KNOCKBACK,
            .buffMask   = BUFFKIND_FIRE,
            .maxpower   = 2.0f,
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
            .cooldown   = 0.0f,
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

const StateFunc ABILITY_STATE_FUNC[ABILITY_STATE_COUNT] = {
    [ABILITY_STATE_IDLE] =
        {
            IdleAbility_State_Update,
            IdleAbility_State_Enter,
            IdleAbility_State_Exit,
            IdleAbility_State_CanExit,
            IdleAbility_State_CanEnter,
        },
    // [ABILITY_STATE_DASH] =
    //     {
    //         ADash_State_Update,
    //         ADash_State_Enter,
    //         ADash_State_Exit,
    //         ADash_State_CanExit,
    //         ADash_State_CanEnter,
    //     },
    // [ABILITY_STATE_FIREBALL] =
    //     {
    //         Fireball_State_Update,
    //         Fireball_State_Enter,
    //         Fireball_State_Exit,
    //         Fireball_State_CanExit,
    //         Fireball_State_CanEnter,
    //         Fireball_State_Draw,
    //     },
    // [ABILITY_STATE_SHIELD] =
    //     {
    //         Shield_State_Update,
    //         Shield_State_Enter,
    //         Shield_State_Exit,
    //         Shield_State_CanExit,
    //         Shield_State_CanEnter,
    //         Shield_State_Draw,
    //     },
    [ABILITY_STATE_CLAW] =
        {
            Claw_State_Update,
            Claw_State_Enter,
            Claw_State_Exit,
            Claw_State_CanExit,
            Claw_State_CanEnter,
        },
    // [ABILITY_STATE_PISTOL] =
    //     {
    //         Pistol_State_Update,
    //         Pistol_State_Enter,
    //         Pistol_State_Exit,
    //         Pistol_State_CanExit,
    //         Pistol_State_CanEnter,
    //     },
    // [ABILITY_STATE_SPINSLASH] =
    //     {
    //         Spinslash_State_Update,
    //         Spinslash_State_Enter,
    //         Spinslash_State_Exit,
    //         Spinslash_State_CanExit,
    //         Spinslash_State_CanEnter,
    //     },
    // [ABILITY_STATE_LASER] =
    //     {
    //         Laser_State_Update,
    //         Laser_State_Enter,
    //         Laser_State_Exit,
    //         Laser_State_CanExit,
    //         Laser_State_CanEnter,
    //         Laser_State_Draw,
    //     },
    // [ABILITY_STATE_WHIP] =
    //     {
    //         Whip_State_Update,
    //         Whip_State_Enter,
    //         Whip_State_Exit,
    //         Whip_State_CanExit,
    //         Whip_State_CanEnter,
    //     },
    // [ABILITY_STATE_FIREBALLVOLLEY] = SCRIPT_STATE_FUNCS,

};

void Ability_Step(World *world, double dt)
{
    float fdt = (float)dt;

    SparseSet_ScAbility *set = Sol_Comp_Set(world, ScAbility);
    for (int i = 0; i < set->cnt; i++)
    {
        int        id      = set->dense[i];
        ScAbility *ability = &set->data[i];
        ScCmd     *cmd     = Sol_Comp_Get(world, id, ScCmd);
        if (!cmd)
            continue;
        for (int j = 0; j < ability->slots; j++)
        {
            int  mask                  = BITC(ACTION_ABILITY1 + j);
            bool held                  = cmd->actionState & mask;
            ability->stateData[j].held = held;
            if (held)
            {
                Sol_Ability_SetState(world, id, ability->action_map[j], j, false);
            }
        }
        if (ABILITY_STATE_FUNC[ability->state].update)
            ABILITY_STATE_FUNC[ability->state].update(world, id, fdt);
    }
}

void Ability_Draw(World *world, double dt)
{
    SparseSet_ScAbility *set = Sol_Comp_Set(world, ScAbility);
    for (int i = 0; i < set->cnt; i++)
    {
        int        id      = set->dense[i];
        ScAbility *ability = &set->data[i];

        if (ABILITY_STATE_FUNC[ability->state].draw)
            ABILITY_STATE_FUNC[ability->state].draw(world, id);
    }
}

bool Sol_Ability_SetState(World *world, int id, AbilityState nextState, int slot, bool force)
{
    if (nextState > ABILITY_STATE_COUNT)
        return false;
    ScAbility       *ability  = Sol_Comp_Get(world, id, ScAbility);
    const StateFunc *prevfunc = &ABILITY_STATE_FUNC[ability->state];
    const StateFunc *nextfunc = &ABILITY_STATE_FUNC[nextState];
    
    if (!force)
    {
        if (!prevfunc->canExit || !prevfunc->canExit(world, id, nextState))
            return false;
        if (!nextfunc->canEnter || !nextfunc->canEnter(world, id, ability->state, nextState, slot))
            return false;
    }
    prevfunc->exit(world, id);
    ability->state                       = nextState;
    ability->activeSlot                  = slot;
    ability->stateData[slot].elapsed     = 0;
    ability->stateData[slot].accum       = 0;
    ability->stateData[slot].lastEntered = world->tickTime;
    nextfunc->enter(world, id);

    return true;
}