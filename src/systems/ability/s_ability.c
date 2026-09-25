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

AbilityConfig ability_base[ABILITYKIND_COUNT] = {
    [ABILITYKIND_IDLE] =
        {
            0,
        },
    [ABILITYKIND_CLAW] =
        {
            .duration   = 0.3f,
            .cooldown   = 1.0f,
            .damage     = 30.0f,
            .buffMask   = BITC(BUFFKIND_FIRE),
            .effectMask = EFFECTMASK_KNOCKBACK,
            .maxpower   = 4.0f,
        },
    [ABILITYKIND_FIREBALL] =
        {
            .duration = 3.0f,
            .recover  = 0.5f,
            .cooldown = 1.0f,
            .damage   = 15.0f,
            // .buffMask = BITC(BUFFKIND_FIRE),
            .maxpower = 1.5f,
        },
    [ABILITYKIND_SHIELD] =
        {
            .duration   = 0.33f,
            .cooldown   = 4.0f,
            .damage     = 15.0f,
            .buffMask   = BITC(BUFFKIND_FIRE),
            .effectMask = EFFECTMASK_KNOCKUP,
        },
};

const u32 abilityState_slot_map[ABILITY_STATE_COUNT] = {
    [ABILITY_STATE_CLAW]        = ABILITYKIND_CLAW,
    [ABILITY_STATE_CLAW_CHARGE] = ABILITYKIND_CLAW,
    [ABILITY_STATE_CLAW_DASH]   = ABILITYKIND_CLAW,

    [ABILITY_STATE_FIREBALL]        = ABILITYKIND_FIREBALL,
    [ABILITY_STATE_FIREBALL_CHARGE] = ABILITYKIND_FIREBALL,
    [ABILITY_STATE_FIREBALL_DASH]   = ABILITYKIND_FIREBALL,

    [ABILITY_STATE_SHIELD] = ABILITYKIND_SHIELD,
};

const u32 abilityslot_state_map[ABILITYKIND_COUNT][3] = {
    [ABILITYKIND_CLAW][0] = ABILITY_STATE_CLAW_CHARGE, //
    [ABILITYKIND_CLAW][1] = ABILITY_STATE_CLAW,        //
    [ABILITYKIND_CLAW][2] = ABILITY_STATE_CLAW_DASH,   //

    [ABILITYKIND_FIREBALL][0] = ABILITY_STATE_FIREBALL_CHARGE, //
    [ABILITYKIND_FIREBALL][1] = ABILITY_STATE_FIREBALL,        //
    [ABILITYKIND_FIREBALL][2] = ABILITY_STATE_FIREBALL_DASH,   //

    [ABILITYKIND_SHIELD][1] = ABILITY_STATE_SHIELD, //
};

extern const AbilityStateFunc ability_idle_state;

extern const AbilityStateFunc ability_claw_state;
extern const AbilityStateFunc ability_claw_charge_state;
extern const AbilityStateFunc ability_claw_dash_state;

extern const AbilityStateFunc ability_fireball_state;
extern const AbilityStateFunc ability_fireball_charge_state;
extern const AbilityStateFunc ability_fireball_dash_state;

extern const AbilityStateFunc ability_shield_state;

extern const AbilityStateFunc ability_dash_state;

const AbilityStateFunc *ability_state_func[ABILITY_STATE_COUNT] = {
    [ABILITY_STATE_IDLE] = &ability_idle_state, //

    [ABILITY_STATE_CLAW]        = &ability_claw_state,        //
    [ABILITY_STATE_CLAW_CHARGE] = &ability_claw_charge_state, //
    [ABILITY_STATE_CLAW_DASH]   = &ability_dash_state,        //

    [ABILITY_STATE_FIREBALL]        = &ability_fireball_state,        //
    [ABILITY_STATE_FIREBALL_CHARGE] = &ability_fireball_charge_state, //
    [ABILITY_STATE_FIREBALL_DASH]   = &ability_dash_state,            //

    [ABILITY_STATE_SHIELD] = &ability_shield_state, //
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
        ScCombat *combat   = Sol_Comp_Get(world, id, ScCombat);
        if (!cmd || !combat || combat->is_dead)
            continue;

        bool is_idle = (ability->state == ABILITY_STATE_IDLE);
        for (int j = 0; j < ABILITY_SLOTS; j++)
        {
            AbilityStateData *data     = &ability->stateData[j];
            data->cooldownRemaining    = fmaxf(0.0f, data->cooldownRemaining - fdt);
            int mask                   = BITC(ACTION_ABILITY1 + j);
            bool held                  = cmd->actionState & mask;
            ability->stateData[j].held = held;
            u32 ability_kind  = ability->slotted_actions[j] ? ability->slotted_actions[j] : ability->base_actions[j];
            u32 ability_state = abilityslot_state_map[ability_kind][abilitybar_slot_map[j]];
            if (held && (is_idle || j == 6))
            {
                if (Sol_Ability_SetState(world, id, ability_state, j, false))
                    break;
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

        const AbilityStateFunc *state_func = ability_state_func[ability->state];
        if (state_func && state_func->draw)
            state_func->draw(world, id, ability, dt);
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
    data->power            = 0;
    data->recoverRemaining = 0;

    if (nextfunc->enter)
        nextfunc->enter(world, id, ability, cmd);

    return true;
}

void Sol_Ability_Equip(World *world, int id, int slot, SolItem item)
{
}

AbilityConfig Sol_Ability_GetConf(SolItem item)
{
    AbilityConfig conf = ability_base[item.abilityKind];
    if (item.abilityKind > 0 && item.abilityKind < ABILITYKIND_COUNT)
    {
        conf.cooldown *= 1.0f - (0.1f * (float)item.rarity);
        conf.recover *= 1.0f - (0.1f * (float)item.rarity);

        conf.damage *= 1.0f + (0.2f * (float)item.rarity);
        conf.maxpower *= 1.0f + (0.5f * (float)item.rarity);
        conf.duration *= 1.0f + (0.1f * (float)item.rarity);
        conf.buffMask |= item.buffMask;
        conf.effectMask |= item.effectMask;
    }
    return conf;
}

AbilityConfig Sol_Ability_GetSlotConf(const ScAbility *ability, int slot)
{
    if (!ability || slot < 0 || slot >= ABILITY_SLOTS)
        return (AbilityConfig){0};
    if (ability->slotted_actions[slot] > 0)
        return Sol_Ability_GetConf(ability->slotted_items[slot]);
    return ability_base[ability->base_actions[slot]];
}

bool Sol_Ability_GetIsDashing(const ScAbility *ability)
{
    return ability->state == ABILITY_STATE_CLAW_DASH || ability->state == ABILITY_STATE_FIREBALL_DASH;
}

float Sol_Ability_GetCurrentBaseDuration(const ScAbility *ability, int slot)
{
}