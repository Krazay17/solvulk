#include "controller/ai/s_ai.h"
#include "world.h"

void Ai_Aggro_Update(World *world, int id, ScAi *ai, float dt)
{
}

void Ai_Aggro_Enter(World *world, int id, ScAi *ai)
{
}

void Ai_Aggro_Exit(World *world, int id, ScAi *ai)
{
}

bool Ai_Aggro_CanExit(World *world, int id, ScAi *ai, u32 next)
{
    return true;
}

bool Ai_Aggro_CanEnter(World *world, int id, ScAi *ai, u32 last)
{
    return true;
}
