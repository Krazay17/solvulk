#include "ability_system.h"
#include "sol_core.h"

// Defined in order of priority (highest priority first)
static const AbilityMapping ability_mappings[] = {
    {ACTION_ABILITY0, ABILITY_STATE_IDLE},
    {ACTION_ABILITY1, ABILITY_STATE_CLAW},
};
#define MAPPING_COUNT (sizeof(ability_mappings) / sizeof(AbilityMapping))

const StateFunc ability_state_func[] = {
    [ABILITY_STATE_IDLE] =
        {
            .update   = IdleAbility_State_Update,
            .enter    = IdleAbility_State_Enter,
            .exit     = IdleAbility_State_Exit,
            .canEnter = IdleAbility_State_CanEnter,
            .canExit  = IdleAbility_State_CanExit,
        },
    [ABILITY_STATE_CLAW] =
        {
            .update   = Claw_State_Update,
            .enter    = Claw_State_Enter,
            .exit     = Claw_State_Exit,
            .canEnter = Claw_State_CanEnter,
            .canExit  = Claw_State_CanExit,
        },
    [ABILITY_STATE_1] = {0},
    [ABILITY_STATE_2] = {0},
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
    world->abilities = calloc(MAX_ENTS, sizeof(CompAbility));
}

void Sol_Ability_Add(World *world, int id, AbilityDesc desc)
{
    CompAbility combat   = {0};
    world->abilities[id] = combat;
    world->masks[id] |= HAS_COMBAT;
}

void Sol_Ability_Step(World *world, double dt, double time)
{
    int required = HAS_COMBAT;
    int count    = world->activeCount;
    for (int i = 0; i < count; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;
        CompAbility    *ability    = &world->abilities[id];
        CompController *controller = &world->controllers[id];

        for (int m = 0; m < MAPPING_COUNT; m++)
        {
            if (controller->actionState & ability_mappings[m].actionBit)
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
    int required = HAS_COMBAT;
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
    CompAbility *ability = &world->abilities[id];
    if (ability->state == nextState)
        return false;
    const StateFunc *prevfunc = &ability_state_func[ability->state];
    if (!prevfunc->canExit || !prevfunc->canExit(world, id))
        return false;
    const StateFunc *nextfunc = &ability_state_func[nextState];
    if (!nextfunc->canEnter || !nextfunc->canEnter(world, id))
        return false;

    ability->stateData[ability->state].elapsed = 0;
    prevfunc->exit(world, id);
    ability->state = nextState;
    nextfunc->enter(world, id);
    ability->stateData[ability->state].lastEntered = (float)Sol_GetState()->gameTime;

    Sol_Debug_Add("state", ability->state);

    return true;
}