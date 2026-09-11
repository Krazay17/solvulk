#include "move3/s_move3.h"
#include "world.h"

void Move_Landing_Update(World *world, int id, ScMove3 *move, ScCmd *cmd, float dt)
{
}

void Move_Landing_Enter(World *world, int id, ScMove3 *move, ScCmd *cmd)
{
    ScBody3 *body = Sol_Comp_Get(world, id, ScBody3);
    body->vel     = GLMS_VEC3_ZERO;
}

void Move_Landing_Exit(World *world, int id, ScMove3 *move, ScCmd *cmd)
{
}

bool Move_Landing_CanExit(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 next)
{
    return move->stateData[move->state].elapsed > MOVE_STATE_FORCES[move->kind][move->state].duration;
}

bool Move_Landing_CanEnter(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 last)
{
    if (Sol_Comp_Has(world, id, ScBody3) && move->groundtime > 0)
    {
        ScBody3 *body       = Sol_Comp_Get(world, id, ScBody3);
        float    dot_ground = glms_vec3_dot(move->stateData[MOVE_FALL].as.fall.velocity, move->groundNorm);
        return dot_ground < -20.0f;
    }
    return false;
}

void Move_Landing_Draw(World *world, int id, ScMove3 *move, ScCmd *cmd, double dt)
{
}
