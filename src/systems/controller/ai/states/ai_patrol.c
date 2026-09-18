#include "controller/ai/s_ai.h"
#include "world.h"

void Ai_Patrol_Update(World *world, int id, ScAi *ai, float dt)
{

}

void Ai_Patrol_Enter(World *world, int id, ScAi *ai)
{

}

void Ai_Patrol_Exit(World *world, int id, ScAi *ai)
{

}

bool Ai_Patrol_CanExit(World *world, int id, ScAi *ai, u32 next)
{
    return true;
}

bool Ai_Patrol_CanEnter(World *world, int id, ScAi *ai, u32 last)
{
    return true;
}

const AiStateFuncs ai_patrol_state = {
    .update= Ai_Patrol_Update,
    .enter = Ai_Patrol_Enter,
    .exit = Ai_Patrol_Enter,
    .canExit = Ai_Patrol_CanExit,
    .canEnter = Ai_Patrol_CanEnter,
};
