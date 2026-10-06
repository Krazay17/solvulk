#include "controller/ai/s_ai.h"
#include "world.h"
#include "estate.h"

static void Update(World *world, int id, ScAi *ai, float dt)
{
}
static void Enter(World *world, int id, ScAi *ai)
{
}
static void Exit(World *world, int id, ScAi *ai)
{
}
static bool CanExit(World *world, int id, ScAi *ai, u32 next)
{
    return true;
}
static bool CanEnter(World *world, int id, ScAi *ai, u32 last)
{
    ScCombat *combat = Sol_Comp_Get(world, id, ScCombat);
    if (combat->is_dead)
        return true;
    return false;
}
static void Draw(World *world, int id, ScAi *ai)
{
}
const AiStateFuncs ai_dead = {Update, Enter, Exit, CanExit, CanEnter, Draw};