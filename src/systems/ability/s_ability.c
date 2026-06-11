#include "sol_core.h"

#include "ability_i.h"

#define MAPPING_COUNT(maps) (sizeof(maps) / sizeof(AbilityMapping))

void Sol_Ability_Init(World *world)
{
    world->stepSystems[world->stepCount++] = Sol_Ability_Step;
    world->abilities                       = calloc(MAX_ENTS, sizeof(CompAbility));

    Ability_Scripts_Init();
}

void Sol_Ability_Add(World *world, int id, AbilityDesc desc)
{
    CompAbility a = {0};
    memcpy(a.bindings, desc.bindings, sizeof(a.bindings));
    for (int i = 0; i < MAX_MAPPED_SKILLS; i++)
    {
        a.stateData[i].lastEntered = -FLT_MAX;
        a.stateData[i].lastExited  = -FLT_MAX;
        a.stateData[i].stage       = 0;
        a.stateData[i].elapsed     = 0.0f;
    }
    a.activeSlot = -1;

    world->masks[id] |= HAS_ABILITY;
    world->abilities[id] = a;
}

void Sol_Ability_Step(World *world, double dt, double time)
{
    int required = HAS_ACTIVE | HAS_ABILITY;
    int count    = world->activeCount;
    for (int i = 0; i < count; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;
        if (Sol_Vital_GetDead(world, id))
        {
            Sol_Ability_SetState(world, id, ABILITY_STATE_IDLE, 0, true);
            continue;
        }
        if (Sol_Buff_HasBuff(world, id, BUFFKIND_STUN))
            continue;
        if (world->replications[id].auth == NETAUTH_REMOTE)
            continue;

        CompAbility *ability = &world->abilities[id];
        SolActions   actions = world->controllers[id].actionState;

        for (int m = 0; m < MAX_MAPPED_SKILLS; m++)
        {
            if (ability->bindings[m].dirtyApply)
            {
                Sol_Ability_Bind(world, id, m, ability->bindings[m].pendingState, ability->bindings[m].pendingRarity,
                                 ability->bindings[m].pendingBonusDamage, ability->bindings[m].pendingBonusBuffs,
                                 ability->bindings[m].pendingBonusEffects);
                ability->bindings[m].dirtyApply = false;
            }
        }
        for (int m = 0; m < MAX_MAPPED_SKILLS; m++)
        {
            SkillBinding *b = &ability->bindings[m];

            if (b->boundState == ABILITY_STATE_IDLE || b->actionBit == ACTION_NONE)
                continue;

            bool pressed               = (actions & b->actionBit) != 0;
            ability->stateData[m].held = pressed;

            if (pressed && ability->activeSlot != m)
            {
                Sol_Ability_SetState(world, id, b->boundState, m, false);
            }
        }

        if (ability->state != ABILITY_STATE_IDLE)
        {
            ability_state_func[ability->state].update(world, id, dt);
        }
    }
}

bool Sol_Ability_SetState(World *world, int id, AbilityState nextState, int slot, bool force)
{
    if (nextState > ABILITY_STATE_COUNT)
        return false;
    CompAbility     *ability  = &world->abilities[id];
    const StateFunc *prevfunc = &ability_state_func[ability->state];
    const StateFunc *nextfunc = &ability_state_func[nextState];

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
    ability->stateData[slot].lastEntered = Sol_GetGameTime();
    nextfunc->enter(world, id);
    return true;
}

AbilityState Sol_Ability_GetState(World *world, int id)
{
    return world->abilities[id].state;
}

void Sol_Ability_RequestBind(World *world, int id, u32 slot, u32 ability, u32 rarity, u32 bonusDamage, u32 bonusBuffs,
                             u32 bonusEffects)
{
    CompAbility *a = &world->abilities[id];
    if (slot >= MAX_MAPPED_SKILLS)
        return;
    SkillBinding *b = &a->bindings[slot];
    if (b->pendingState == ability && b->pendingRarity == rarity)
        return;
    b->pendingState        = ability;
    b->pendingRarity       = rarity;
    b->pendingBonusDamage  = bonusDamage;
    b->pendingBonusBuffs   = bonusBuffs;
    b->pendingBonusEffects = bonusEffects;

    b->dirtyApply = true;
    b->dirtySend  = true;
}

void Sol_Ability_Bind(World *world, int id, u32 slot, u32 ability, u32 rarity, u32 bonusDamage, u32 bonusBuffs,
                      u32 bonusEffects)
{
    CompAbility *a = &world->abilities[id];
    if (slot >= MAX_MAPPED_SKILLS)
        return;

    SkillBinding *b      = &a->bindings[slot];
    b->boundState        = ability;
    b->boundRarity       = rarity;
    b->boundBonusDamage  = bonusDamage;
    b->boundBonusBuffs   = bonusBuffs;
    b->boundBonusEffects = bonusEffects;

    b->pendingState        = ability;
    b->pendingRarity       = rarity;
    b->pendingBonusDamage  = bonusDamage;
    b->pendingBonusBuffs   = bonusBuffs;
    b->pendingBonusEffects = bonusEffects;

    AbilityData *data = &a->stateData[slot];
    data->cooldown    = ability_config[ability][rarity].cooldown;
    data->duration    = ability_config[ability][rarity].duration;
    data->damage      = ability_config[ability][rarity].damage + bonusDamage;
    data->buffs       = ability_config[ability][rarity].buffMask | bonusBuffs;
    data->effects     = ability_config[ability][rarity].effectMask | bonusEffects;
}

const char *Sol_Ability_GetNameString(u32 ability)
{
    return ability_config[ability]->name;
}