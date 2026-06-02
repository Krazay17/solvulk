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
    memcpy(a.ability_mappings, desc.abilityMapping, sizeof(AbilityMapping) * ABILITY_STATE_COUNT);
    for (int i = 0; i < ABILITY_STATE_COUNT; i++)
    {
        a.stateData[i].lastEntered = -FLT_MAX;
        a.stateData[i].lastExited  = -FLT_MAX;
    }

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
            Sol_Ability_SetState(world, id, ABILITY_STATE_IDLE, true);
            continue;
        }
        if (world->replications[id].auth == NETAUTH_REMOTE)
            continue;
        CompAbility *ability = &world->abilities[id];

        SolActions actions = Sol_GetActions(world, id);
        for (int m = 0; m < ABILITY_STATE_COUNT; m++)
        {
            AbilityMapping *map = &ability->ability_mappings[m];
            if (map->actionBit == 0)
                continue;

            bool pressed                              = (actions & map->actionBit) != 0;
            ability->stateData[map->targetState].held = pressed;

            if (pressed)
            {
                Sol_Ability_SetState(world, id, map->targetState, false);
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

void Sol_Ability_Tick(World *world, double dt, double time)
{
    int required = HAS_ABILITY;
    int count    = world->activeCount;
    for (int i = 0; i < count; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;
    }
}

bool Sol_Ability_SetState(World *world, int id, AbilityState nextState, bool force)
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
        if (!nextfunc->canEnter || !nextfunc->canEnter(world, id, ability->state, nextState))
            return false;
    }

    prevfunc->exit(world, id);
    ability->state                                 = nextState;
    ability->stateData[ability->state].elapsed     = 0;
    ability->stateData[ability->state].accum       = 0;
    ability->stateData[ability->state].lastEntered = (float)Sol_GetGameTime();
    nextfunc->enter(world, id);
    return true;
}

AbilityState Sol_Ability_GetState(World *world, int id)
{
    return world->abilities[id].state;
}
