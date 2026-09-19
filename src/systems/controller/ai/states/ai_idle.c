#include "controller/ai/s_ai.h"
#include "world.h"
#include "sol_math.h"

void Ai_Idle_Update(World *world, int id, ScAi *ai, float dt)
{
}

void Ai_Idle_Enter(World *world, int id, ScAi *ai)
{
    ScCmd *cmd = Sol_Comp_Get(world, id, ScCmd);
    if (!cmd)
        return;
    cmd->wishdir     = (vec3s){0, 0, 0};
    cmd->actionState = 0;
}

void Ai_Idle_Exit(World *world, int id, ScAi *ai)
{
}

bool Ai_Idle_CanExit(World *world, int id, ScAi *ai, u32 next)
{
    return true;
}

bool Ai_Idle_CanEnter(World *world, int id, ScAi *ai, u32 last)
{
    return true;
}

const AiStateFuncs ai_idle_state = {
    .update   = Ai_Idle_Update,
    .enter    = Ai_Idle_Enter,
    .exit     = Ai_Idle_Exit,
    .canExit  = Ai_Idle_CanExit,
    .canEnter = Ai_Idle_CanEnter,
};
