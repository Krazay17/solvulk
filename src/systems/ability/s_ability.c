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

const AbilityConfig ability_base[ABILITY_STATE_COUNT] = {
    [ABILITY_STATE_IDLE] =
        {
            0,
        },
    [ABILITY_STATE_CLAW_CHARGE] =
        {
            .duration = 0.45f,
            .cooldown = 1.0f,
            .maxpower = 1.0f,
            .speed    = 1.0f,
            .damage =
                {
                    .amount     = 25.0f,
                    .effectMask = EFFECTMASK_KNOCKBACK | EFFECTMASK_REFLECTPROJECTILE,
                },
        },
    [ABILITY_STATE_FIREBALL_CHARGE] =
        {
            .duration = 0.6f,
            .maxpower = 1.0f,
            .speed    = 1.0f,
            .cooldown = 1.0f,
            .damage =
                {
                    .amount   = 25.0f,
                    .buffMask = BITC(BUFFKIND_FIRE),
                },
        },
    [ABILITY_STATE_SHIELD_CHARGE] =
        {
            .duration = 0.6f,
            .maxpower = 1.0f,
            .speed    = 1.0f,
            .cooldown = 1.0f,
            .damage   = 25.0f,
            .damage =
                {
                    .amount     = 25.0f,
                    .effectMask = EFFECTMASK_KNOCKUP | EFFECTMASK_REFLECTPROJECTILE,
                },
        },
    [ABILITY_STATE_WHIP_CHARGE] =
        {
            .duration = 0.6f,
            .maxpower = 1.0f,
            .speed    = 1.0f,
            .cooldown = 1.0f,
            .damage =
                {
                    .amount = 25.0f,
                },
        },
    [ABILITY_STATE_CLAW_DASH] =
        {
            .duration = 0.6f,
            .cooldown = 4.0f,
            .damage   = 25.0f,
            .maxpower = 1.0f,
            .speed    = 1.0f,
            .damage =
                {
                    .amount = 25.0f,
                    .effectMask = EFFECTMASK_KNOCKBACK | EFFECTMASK_REFLECTPROJECTILE,
                },
        },
    [ABILITY_STATE_FIREBALL] =
        {
            .duration = 0.6f,
            .maxpower = 1.1f,
            .speed    = 1.0f,
            .cooldown = 1.0f,
            .damage =
                {
                    .amount   = 25.0f,
                    .buffMask = BITC(BUFFKIND_FIRE),
                },

        },
    [ABILITY_STATE_SHIELD] =
        {
            .duration = 0.4f,
            .cooldown = 1.0f,
            .damage =
                {
                    .amount     = 25.0f,
                    .effectMask = EFFECTMASK_KNOCKUP | EFFECTMASK_REFLECTPROJECTILE,
                },
        },
    [ABILITY_STATE_SHIELD_DASH] =
        {
            .duration = 0.3f,
            .cooldown = 1.0f,
            .damage =
                {
                    .amount     = 25.0f,
                    .effectMask = EFFECTMASK_KNOCKUP,
                },
        },
};

const u32 slot_kind_map[7] = {0, 0, 1, 1, 1, 1, 2};

const u32 abilityslot_state_map[ABILITYKIND_COUNT][3] = {
    [ABILITYKIND_CLAW][0] = ABILITY_STATE_CLAW_CHARGE, //
    [ABILITYKIND_CLAW][1] = ABILITY_STATE_CLAW,        //
    [ABILITYKIND_CLAW][2] = ABILITY_STATE_CLAW_DASH,   //

    [ABILITYKIND_FIREBALL][0] = ABILITY_STATE_FIREBALL_CHARGE, //
    [ABILITYKIND_FIREBALL][1] = ABILITY_STATE_FIREBALL,        //
    [ABILITYKIND_FIREBALL][2] = ABILITY_STATE_FIREBALL_DASH,   //

    [ABILITYKIND_SHIELD][0] = ABILITY_STATE_SHIELD_CHARGE, //
    [ABILITYKIND_SHIELD][1] = ABILITY_STATE_SHIELD,        //
    [ABILITYKIND_SHIELD][2] = ABILITY_STATE_SHIELD_DASH,   //

    [ABILITYKIND_WHIP][0] = ABILITY_STATE_WHIP_CHARGE, //
};

extern const AbilityStateFunc ability_idle_state;

extern const AbilityStateFunc ability_claw_state;
extern const AbilityStateFunc ability_claw_charge_state;
extern const AbilityStateFunc ability_claw_dash_state;

extern const AbilityStateFunc ability_fireball_state;
extern const AbilityStateFunc ability_fireball_charge_state;
extern const AbilityStateFunc ability_fireball_dash_state;

extern const AbilityStateFunc ability_shield_charge_state;
extern const AbilityStateFunc ability_shield_state;

extern const AbilityStateFunc ability_whip_charge;

extern const AbilityStateFunc ability_dash_state;

const AbilityStateFunc *ability_state_func[ABILITY_STATE_COUNT] = {
    [ABILITY_STATE_IDLE] = &ability_idle_state, //

    [ABILITY_STATE_CLAW]        = &ability_claw_state,        //
    [ABILITY_STATE_CLAW_CHARGE] = &ability_claw_charge_state, //
    [ABILITY_STATE_CLAW_DASH]   = &ability_claw_dash_state,   //

    [ABILITY_STATE_FIREBALL]        = &ability_fireball_state,        //
    [ABILITY_STATE_FIREBALL_CHARGE] = &ability_fireball_charge_state, //
    [ABILITY_STATE_FIREBALL_DASH]   = &ability_dash_state,            //

    [ABILITY_STATE_SHIELD]        = &ability_shield_state, //
    [ABILITY_STATE_SHIELD_CHARGE] = &ability_shield_charge_state,
    [ABILITY_STATE_SHIELD_DASH]   = &ability_dash_state, //

    [ABILITY_STATE_WHIP_CHARGE] = &ability_whip_charge, //
};

static inline u32 Get_SlotState(const ScAbility *ability, int slot)
{
    u32 kind = ability->slotted_actions[slot] > 0 ? ability->slotted_actions[slot] : ability->base_actions[slot];
    return abilityslot_state_map[kind][slot_kind_map[slot]];
}

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
            if (held && (is_idle || j == 6))
            {
                if (Sol_Ability_SetState(world, id, Get_SlotState(ability, j), j, false))
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
    data->hitPause         = 0;
    data->hitPauseDr       = 0;

    if (nextfunc->enter)
        nextfunc->enter(world, id, ability, cmd);

    return true;
}

void Sol_Ability_Equip(World *world, int id, int slot, SolItem item)
{
}

AbilityConfig Sol_Ability_GetSlotConf(const ScAbility *ability, int slot)
{
    if (!ability || slot < 0 || slot >= ABILITY_SLOTS)
        return (AbilityConfig){0};
    AbilityConfig conf = ability_base[Get_SlotState(ability, slot)];
    if (ability->slotted_actions[slot] > 0)
        return Sol_Item_ApplyMods(conf, ability->slotted_items[slot]);
    return conf;
}

bool Sol_Ability_GetIsDashing(const ScAbility *ability)
{
    return ability->state == ABILITY_STATE_CLAW_DASH || ability->state == ABILITY_STATE_FIREBALL_DASH ||
           (ability->state == ABILITY_STATE_CLAW_CHARGE && (ability->stateData[ability->activeSlot].stage > 0));
}

float Sol_Ability_GetCurrentBaseDuration(const ScAbility *ability, int slot)
{
    return ability_base[Get_SlotState(ability, slot)].duration;
}

DefendResult Sol_Ability_TryDefend(World *world, int id, SolHit *hit)
{
    ScAbility *ability = Sol_Comp_Get(world, id, ScAbility);
    if (!ability)
        return DEFENDKIND_NONE;

    const AbilityStateFunc *f = ability_state_func[ability->state];
    if (!f || !f->defend)
        return DEFENDKIND_NONE;

    return f->defend(world, id, ability, hit);
}