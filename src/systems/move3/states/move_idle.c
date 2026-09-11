#include "move3/s_move3.h"
#include "world.h"
#include "sol_math.h"

void Move_Idle_Update(World *world, int id, ScMove3 *move, ScCmd *cmd, float dt)
{
    move->gravityMod = 0.0f;
}

void Move_Idle_Enter(World *world, int id, ScMove3 *move, ScCmd *cmd)
{
    ScBody3 *body      = Sol_Comp_Get(world, id, ScBody3);
    move->targetHeight = move->baseHeight;
}

void Move_Idle_Exit(World *world, int id, ScMove3 *move, ScCmd *cmd)
{
    // todo
}

bool Move_Idle_CanExit(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 next)
{
    // todo
    return true;
}

bool Move_Idle_CanEnter(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 last)
{
    return true;
}
