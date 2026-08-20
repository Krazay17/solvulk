#pragma once
#include "sol/types.h"

typedef enum
{
    AISTATE_IDLE,
    AISTATE_PATROL,
    AISTATE_SEARCH,
    AISTATE_AGGRO,
    AISTATE_RETREAT,
    AISTATE_COUNT,
} AiState;
typedef enum
{
    AIKIND_WIZARD,
} AiKind;
typedef struct
{
    float lastEntered, elapsed, duration, accum;
    float attacktimer;
} AiStateData;

typedef struct CompAi
{
    vec3s       dirToTarget;
    AiState     state;
    u32         target, justHitUs;
    float       distToTarget, dropAggroTimer, lastHit;
    AiStateData stateData[AISTATE_COUNT];
} CompAi;

void Sol_Ai_Init(World *world);

CompAi *Sol_Ai_Add(World *world, int id);
CompAi *Sol_Ai_Get(World *world, int id);
void    Sol_Ai_Remove(World *world, int id);

int Sol_Ai_FindTarget(World *world, int id);