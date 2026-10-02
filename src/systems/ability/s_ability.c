/*
 * File: s_ability.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-04
 *
 */
#include "s_ability.h"
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
    [ABILITY_STATE_BOLT_CHARGE] =
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
    [ABILITY_STATE_SHIELD_DASH] =
        {
            .duration = 0.3f,
            .cooldown = 2.0f,
            .damage =
                {
                    .amount     = 25.0f,
                    .effectMask = EFFECTMASK_KNOCKUP,
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
                    .amount     = 25.0f,
                    .effectMask = EFFECTMASK_KNOCKBACK | EFFECTMASK_REFLECTPROJECTILE,
                },
        },
    [ABILITY_STATE_FIREBALL] =
        {
            .duration = 0.6f,
            .maxpower = 1.1f,
            .speed    = 1.0f,
            .cooldown = 6.0f,
            .damage =
                {
                    .amount   = 25.0f,
                    .buffMask = BITC(BUFFKIND_FIRE),
                },

        },
    [ABILITY_STATE_SHIELD] =
        {
            .duration = 0.75f,
            .cooldown = 12.0f,
            .damage =
                {
                    .amount     = 25.0f,
                    .effectMask = EFFECTMASK_KNOCKUP | EFFECTMASK_REFLECTPROJECTILE,
                },
        },
};
const u32 slot_kind_map[7]                            = {0, 0, 0, 0, 2, 1, 1};
const u32 abilityslot_state_map[ABILITYKIND_COUNT][3] = {
    [ABILITYKIND_CLAW][0] = ABILITY_STATE_CLAW,        //
    [ABILITYKIND_CLAW][1] = ABILITY_STATE_CLAW_CHARGE, //
    [ABILITYKIND_CLAW][2] = ABILITY_STATE_CLAW_DASH,   //

    [ABILITYKIND_FIREBALL][0] = ABILITY_STATE_FIREBALL,        //
    [ABILITYKIND_FIREBALL][1] = ABILITY_STATE_FIREBALL_CHARGE, //
    [ABILITYKIND_FIREBALL][2] = ABILITY_STATE_FIREBALL_DASH,   //

    [ABILITYKIND_SHIELD][0] = ABILITY_STATE_SHIELD,        //
    [ABILITYKIND_SHIELD][1] = ABILITY_STATE_SHIELD_CHARGE, //
    [ABILITYKIND_SHIELD][2] = ABILITY_STATE_SHIELD_DASH,   //

    [ABILITYKIND_BOLT][1] = ABILITY_STATE_BOLT_CHARGE, //
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

extern const AbilityStateFunc ability_bolt_charge;

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

    [ABILITY_STATE_BOLT_CHARGE] = &ability_bolt_charge, //
};

static inline u32 Get_SlotState(const ScAbility *ability, int slot)
{
    u32 kind = ability->slotted_actions[slot] > 0 ? ability->slotted_actions[slot] : ability->base_actions[slot];
    return abilityslot_state_map[kind][slot_kind_map[slot]];
}

#define SHARED_LOCKOUT_COUNT 5
#define SHARED_LOCKOUT ((1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4))
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

        // Build busy mask
        u32 busy_mask = 0;
        for (int j = 0; j < ABILITY_SLOTS; j++)
        {
            if (ability->state[j] != 0)
                busy_mask |= 1 << j;
        }
        for (int j = 0; j < ABILITY_SLOTS; j++)
        {
            int mask                   = 1 << (ACTION_ABILITY1 + j);
            bool held                  = cmd->actionState & mask;
            ability->stateData[j].held = held;

            if (busy_mask & SHARED_LOCKOUT || busy_mask & 1 << j)
                continue;

            if (held)
            {
                if (Sol_Ability_SetState(world, id, Get_SlotState(ability, j), j, false))
                    break;
            }
        }

        for (int j = 0; j < ABILITY_SLOTS; j++)
        {
            AbilityStateData *data             = &ability->stateData[j];
            data->cooldownRemaining            = fmaxf(0.0f, data->cooldownRemaining - fdt);
            const AbilityStateFunc *state_func = ability_state_func[ability->state[j]];
            if (state_func && state_func->update)
                state_func->update(world, id, ability, cmd, j, fdt);
        }
    }
}

void Ability_Draw(World *world, double dt)
{
    SparseSet_ScAbility *set = Sol_Comp_Set(world, ScAbility);
    for (int i = 0; i < set->cnt; i++)
    {
        int id             = set->dense[i];
        ScAbility *ability = &set->data[i];

        for (int j = 0; j < ABILITY_SLOTS; j++)
        {
            const AbilityStateFunc *state_func = ability_state_func[ability->state[j]];
            if (state_func && state_func->draw)
                state_func->draw(world, id, ability, j, dt);
        }
    }
}

bool Sol_Ability_SetState(World *world, int id, AbilityState target_state, int slot, bool force)
{
    if (target_state >= ABILITY_STATE_COUNT)
        return false;
    ScAbility *ability               = Sol_Comp_Get(world, id, ScAbility);
    ScCmd *cmd                       = Sol_Comp_Get(world, id, ScCmd);
    const AbilityStateFunc *prevfunc = ability_state_func[ability->state[slot]];
    const AbilityStateFunc *nextfunc = ability_state_func[target_state];
    if (!prevfunc || !nextfunc)
        return false;

    if (!force)
    {
        if (!prevfunc->canExit || !prevfunc->canExit(world, id, ability, cmd, slot))
            return false;
        if (!nextfunc->canEnter || !nextfunc->canEnter(world, id, ability, cmd, slot))
            return false;
    }
    if (slot < 5)
    {
        if (ability->state[5] != 0)
            Sol_Ability_SetState(world, id, 0, 5, true);
        if (ability->state[6] != 0)
            Sol_Ability_SetState(world, id, 0, 6, true);
    }

    if (prevfunc->exit)
        prevfunc->exit(world, id, ability, cmd, slot);

    ability->state[slot] = target_state;

    AbilityStateData *data = &ability->stateData[slot];
    data->elapsed          = 0;
    data->accum            = 0;
    data->stage            = 0;
    data->power            = 0;
    data->hitPause         = 0;
    data->hitPauseDr       = 0;

    if (nextfunc->enter)
        nextfunc->enter(world, id, ability, cmd, slot);

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
    bool isDashing = false;
    for (int i = 0; i < ABILITY_SLOTS; i++)
    {
        if (ability->state[i] == ABILITY_STATE_CLAW_DASH || ability->state[i] == ABILITY_STATE_FIREBALL_DASH ||
            (ability->state[i] == ABILITY_STATE_CLAW_CHARGE && (ability->stateData[i].stage > 0)) ||
            (ability->state[i] == ABILITY_STATE_BOLT_CHARGE && ability->stateData[i].as.bolt.bolt_state == 1))
            isDashing = true;
    }
    return isDashing;
}

float Sol_Ability_GetCurrentBaseDuration(const ScAbility *ability, int slot)
{
    return ability_base[Get_SlotState(ability, slot)].duration;
}

DefendResult Sol_Ability_TryDefend(World *world, int id, SolHit *hit)
{
    ScAbility *ability = Sol_Comp_Get(world, id, ScAbility);
    if (ability)
    {
        for (int i = 0; i < ABILITY_SLOTS; i++)
        {
            const AbilityStateFunc *f = ability_state_func[ability->state[i]];
            if (!f || !f->defend)
                return DEFENDKIND_NONE;

            return f->defend(world, id, ability, hit, i);
        }
    }
    return DEFENDKIND_NONE;
}