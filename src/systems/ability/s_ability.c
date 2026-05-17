#include "sol_core.h"

#include "ability_i.h"

// Defined in order of priority (highest priority first)
static const AbilityMapping ability_mappings[] = {
    {ACTION_ABILITY0, ABILITY_STATE_IDLE},
    {ACTION_ABILITY1, ABILITY_STATE_CLAW},
    {ACTION_DASH, ABILITY_STATE_DASH},
    {ACTION_ABILITY2, ABILITY_STATE_SHIELD},
};
#define MAPPING_COUNT (sizeof(ability_mappings) / sizeof(AbilityMapping))

const StateFunc ability_state_func[] = {
    [ABILITY_STATE_IDLE] =
        {
            .update   = IdleAbility_State_Update,
            .enter    = IdleAbility_State_Enter,
            .exit     = IdleAbility_State_Exit,
            .canExit  = IdleAbility_State_CanExit,
            .canEnter = IdleAbility_State_CanEnter,
        },
    [ABILITY_STATE_DASH] =
        {
            .update   = ADash_State_Update,
            .enter    = ADash_State_Enter,
            .exit     = ADash_State_Exit,
            .canExit  = ADash_State_CanExit,
            .canEnter = ADash_State_CanEnter,
        },
    [ABILITY_STATE_CLAW] =
        {
            .update   = Claw_State_Update,
            .enter    = Claw_State_Enter,
            .exit     = Claw_State_Exit,
            .canExit  = Claw_State_CanExit,
            .canEnter = Claw_State_CanEnter,
        },
    [ABILITY_STATE_SHIELD] =
        {
            Shield_State_Update,
            Shield_State_Enter,
            Shield_State_Exit,
            Shield_State_CanExit,
            Shield_State_CanEnter,
        },
    [ABILITY_STATE_3] = {0},
    [ABILITY_STATE_4] = {0},
    [ABILITY_STATE_5] = {0},
    [ABILITY_STATE_6] = {0},
    [ABILITY_STATE_7] = {0},
    [ABILITY_STATE_8] = {0},
    [ABILITY_STATE_9] = {0},
};

void Sol_Ability_Init(World *world)
{
    world->stepSystems[world->stepCount++] = Sol_Ability_Step;

    world->abilities = calloc(MAX_ENTS, sizeof(CompAbility));
}

void Sol_Ability_Add(World *world, int id, AbilityDesc desc)
{
    CompAbility combat   = {0};
    world->abilities[id] = combat;
    world->masks[id] |= HAS_ABILITY;
}

void Sol_Ability_Step(World *world, double dt, double time)
{
    int required = HAS_ABILITY;
    int count    = world->activeCount;
    for (int i = 0; i < count; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;
        CompAbility *ability = &world->abilities[id];

        for (int m = 0; m < MAPPING_COUNT; m++)
        {
            if (Sol_GetActions(world, id) & ability_mappings[m].actionBit)
                if (Sol_Ability_SetState(world, id, ability_mappings[m].targetState))
                    break;
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

bool Sol_Ability_SetState(World *world, int id, AbilityState nextState)
{
    if (nextState > ABILITY_STATE_COUNT)
        return false;
    CompAbility     *ability  = &world->abilities[id];
    const StateFunc *prevfunc = &ability_state_func[ability->state];
    if (!prevfunc->canExit || !prevfunc->canExit(world, id, nextState))
        return false;
    const StateFunc *nextfunc = &ability_state_func[nextState];
    if (!nextfunc->canEnter || !nextfunc->canEnter(world, id, ability->state))
        return false;

    prevfunc->exit(world, id);
    ability->state                                 = nextState;
    ability->stateData[ability->state].elapsed     = 0;
    ability->stateData[ability->state].accum       = 0;
    ability->stateData[ability->state].lastEntered = (float)Sol_GetState()->gameTime;
    nextfunc->enter(world, id);
    return true;
}
AbilityState Sol_Ability_GetState(World *world, int id)
{
    return world->abilities[id].state;
}