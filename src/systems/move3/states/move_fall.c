#include "move3/s_move3.h"
#include "world.h"
#include "sol_math.h"

void Move_Fall_Update(World *world, int id, ScMove3 *move, ScCmd *cmd, float dt)
{
    ScBody3 *body                                 = Sol_Comp_Get(world, id, ScBody3);
    move->stateData[move->state].as.fall.velocity = body->vel;
}

void Move_Fall_Enter(World *world, int id, ScMove3 *move, ScCmd *cmd)
{
}

void Move_Fall_Exit(World *world, int id, ScMove3 *move, ScCmd *cmd)
{
    move->stateData[MOVE_FALL].as.fall.velocity = GLMS_VEC3_ZERO;
}

bool Move_Fall_CanExit(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 next)
{
    return true;
}

bool Move_Fall_CanEnter(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 last)
{
    if (move->airtime > 0)
        return true;
    return false;
}
