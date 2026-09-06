#pragma once
#include "sol/base.h"

typedef struct World   World;
typedef struct ScMove3 ScMove3;
typedef struct ScCmd   ScCmd;

typedef void (*StateUpdate)(World *world, int id, float dt);
typedef void (*StateEnter)(World *world, int id);
typedef void (*StateExit)(World *world, int id);
typedef bool (*StateCanExit)(World *world, int id, u32 next);
typedef bool (*StateCanEnter)(World *world, int id, u32 last, u32 next, int slot);
typedef void (*StateDraw)(World *world, int id);

typedef void (*MoveStateUpdate)(World *world, int id, ScMove3 *move, ScCmd *cmd, float dt);
typedef void (*MoveStateEnter)(World *world, int id, ScMove3 *move, ScCmd *cmd);
typedef void (*MoveStateExit)(World *world, int id, ScMove3 *move, ScCmd *cmd);
typedef bool (*MoveStateCanExit)(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 next);
typedef bool (*MoveStateCanEnter)(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 last);
typedef void (*MoveStateDraw)(World *world, int id, ScMove3 *move, ScCmd *cmd);

typedef struct
{
    StateUpdate   update;
    StateEnter    enter;
    StateExit     exit;
    StateCanExit  canExit;
    StateCanEnter canEnter;
    StateDraw     draw;
} StateFunc;

typedef struct
{
    MoveStateUpdate   update;
    MoveStateEnter    enter;
    MoveStateExit     exit;
    MoveStateCanExit  canExit;
    MoveStateCanEnter canEnter;
    MoveStateDraw     draw;
} MoveStateFunc;

// void State_Update(World *world, int id, float dt);
// void State_Enter(World *world, int id);
// void State_Exit(World *world, int id);
// bool State_CanExit(World *world, int id, u32 nextState);
// bool State_CanEnter(World *world, int id, u32 lastState, u32 nextState, int slot);
// void State_Draw(World *world, int id, double dt);

// bool SetState(World *world, int id, u32 nextState)
// {
//     void *currentState;
//     if (currentState == nextState)
//         return false;
//     const StateFunc *prevfunc = &state_func[currentState];
//     if (!prevfunc->canExit(world, id, nextState))
//         return false;
//     const StateFunc *nextfunc = &state_func[nextState];
//     if (!nextfunc->canEnter(world, id, currentState))
//         return false;

//     prevfunc->exit(world, id);
//     currentState = nextState;
//     nextfunc->enter(world, id);

//     return true;
// }

// #include "sol_core.h"

// #include "ability/si_ability.h"

// #define COOLDOWN 2.0f
// #define DURATION 1.0f

// void Spinslash_State_Update(World *world, int id, float dt)
// {
//     CompAbility *ability = Sol_Ability_Get(world, id);
//     AbilityStateData *data    = &ability->stateData[ability->activeSlot];
//     data->elapsed += dt;

// }

// void Spinslash_State_Enter(World *world, int id)
// {
//     Sol_Model_PlayAnim(world, id, (AnimDesc){.anim = ANIM_ABILITY0, .layerId = ANIM_LAYER_OVERRIDE, .oneShot =
//     true});
// }

// void Spinslash_State_Exit(World *world, int id)
// {
//     Sol_Model_PlayAnim(world, id, (AnimDesc){.layerId = ANIM_LAYER_OVERRIDE});
// }

// bool Spinslash_State_CanExit(World *world, int id, u32 next)
// {
//     CompAbility *ability = Sol_Ability_Get(world, id);
//     AbilityStateData *data    = &ability->stateData[ability->activeSlot];

//     return data->elapsed > DURATION;
// }

// bool Spinslash_State_CanEnter(World *world, int id, u32 last, u32 next, u32 slot)
// {
//     CompAbility *ability = Sol_Ability_Get(world, id);
//     AbilityStateData *data    = &ability->stateData[slot];
//     return !(data->lastExited + COOLDOWN > solState.appTime);
// }
