#pragma once

#include "controller.h"
#include "estate.h"

typedef struct CompController
{
    vec3s          lookdir, wishdir, aimdir, aimpos, aimHitPos;
    SolActions     actionState;
    float          yaw, pitch;
    float          zoom;
    u32            aimHitEnt;
    ControllerKind kind;
} CompController;

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

bool         Ai_SetState(World *world, int id, AiState nextState);
AiStateData *Ai_GetStatedata(World *world, int id);
u32          AiController_FindTarget(World *world, int id);

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
