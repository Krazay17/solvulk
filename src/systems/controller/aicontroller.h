#pragma once
#include "sol/types.h"

#include "estate.h"

typedef enum
{
    AICONTROLLERKIND_WIZARD,
} AiControllerKind;

typedef enum
{
    AISTATE_IDLE,
    AISTATE_PATROL,
    AISTATE_SEARCH,
    AISTATE_AGGRO,
    AISTATE_RETREAT,
    AISTATE_COUNT,
} AiState;

typedef struct
{
    float lastEntered, elapsed, duration, accum;
    float attacktimer;
} AiStateData;
typedef struct CompAiController
{
    vec3s       dirToTarget;
    AiState     state;
    u32         target, justHitUs, lastHit;
    float       distToTarget, dropAggroTimer;
    AiStateData stateData[AISTATE_COUNT];
} CompAiController;

extern const StateFunc aistate_func[AISTATE_COUNT];

void Sol_AiController_Init(World *world);
void Sol_AiController_Add(World *world, int id, AiControllerKind kind);
void Sol_AiController_Clear(World *world, int id);

bool         Ai_SetState(World *world, int id, AiState nextState);
AiStateData *Ai_GetStatedata(World *world, int id);
u32          AiController_FindTarget(World *world, int id);

void Sol_AiController_SetLastHit(World *world, int id, int source, u32 damage);
void Sol_AiController_TargetDied(World *world, int id, int target);

void Patrol_State_Update(World *world, int id, float dt);
void Patrol_State_Enter(World *world, int id);
void Patrol_State_Exit(World *world, int id);
bool Patrol_State_CanExit(World *world, int id);
bool Patrol_State_CanEnter(World *world, int id);

void Idle_State_Update(World *world, int id, float dt);
void Idle_State_Enter(World *world, int id);
void Idle_State_Exit(World *world, int id);
bool Idle_State_CanExit(World *world, int id);
bool Idle_State_CanEnter(World *world, int id);

void Aggro_State_Update(World *world, int id, float dt);
void Aggro_State_Enter(World *world, int id);
void Aggro_State_Exit(World *world, int id);
bool Aggro_State_CanExit(World *world, int id);
bool Aggro_State_CanEnter(World *world, int id);
