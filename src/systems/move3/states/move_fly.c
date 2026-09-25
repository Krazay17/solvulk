#include "move3/s_move3.h"
#include "world.h"
#include "sol_math.h"

void Move_Fly_Update(World *world, int id, ScMove3 *move, ScCmd *cmd, float dt)
{
}

void Move_Fly_Enter(World *world, int id, ScMove3 *move, ScCmd *cmd)
{
}

void Move_Fly_Exit(World *world, int id, ScMove3 *move, ScCmd *cmd)
{
}


bool Move_Fly_CanExit(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 next)
{
    return true;
}

bool Move_Fly_CanEnter(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 last)
{
    return false;
}

const MoveStateFuncs move_fly_funcs = {
    .update   = Move_Fly_Update,
    .enter    = Move_Fly_Enter,
    .exit     = Move_Fly_Exit,
    .canExit  = Move_Fly_CanExit,
    .canEnter = Move_Fly_CanEnter,
};
