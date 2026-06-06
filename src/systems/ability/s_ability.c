#include "sol_core.h"

#include "ability_i.h"

#define MAPPING_COUNT(maps) (sizeof(maps) / sizeof(AbilityMapping))

static void AbilityCards(World *world, double dt, double time);

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
        if (world->replications[id].auth == NETAUTH_REMOTE)
            continue;
        CompAbility *ability = &world->abilities[id];

        SolActions actions = Sol_GetActions(world, id);
        for (int m = 0; m < MAX_MAPPED_SKILLS; m++)
        {
            SolActions   actionBit   = ability->bindings[m].actionBit;
            AbilityState targetState = ability->bindings[m].targetState;
            if (targetState == ABILITY_STATE_IDLE || actionBit == ACTION_NONE)
                continue;
            bool pressed = (actions & actionBit) != 0;

            ability->stateData[m].held = pressed;

            if (pressed)
            {
                Sol_Ability_SetState(world, id, targetState, m, false);
            }
        }

        if (ability->state != ABILITY_STATE_IDLE)
        {
            const StateFunc *funcs = &ability_state_func[ability->state];
            if (funcs && funcs->update)
                funcs->update(world, id, dt);
        }
    }
}

bool Sol_Ability_SetState(World *world, int id, AbilityState nextState, u32 slot, bool force)
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

void Sol_Ability_SetAbility(World *world, int id, u32 slotIndex, AbilityState ability)
{
    if (slotIndex < MAX_MAPPED_SKILLS)
    {
        if (world->abilities[id].bindings[slotIndex].targetState != ability)
        {
            world->abilities[id].bindings[slotIndex].targetState = ability;
            world->abilities[id].stateData[slotIndex].charge = 0;
        }
    }
}