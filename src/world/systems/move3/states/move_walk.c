#include "move3/s_move3.h"

#include "world.h"
#include "sol_math.h"

void Move_Walk_Update(World *world, int id, ScMove3 *move, ScCmd *cmd, float dt)
{
    ScXform *xform = Sol_Comp_Get(world, id, ScXform);

    float x                                   = cmd->wishdir.x;
    float z                                   = cmd->wishdir.z;
    vec3s rot                                 = Sol_RotFromQuat(xform->rot);
    move->stateData[MOVE_WALK].as.walk.strafe = Sol_GetStrafedir(x, z, rot.x, rot.z);
}

void Move_Walk_Enter(World *world, int id, ScMove3 *move, ScCmd *cmd)
{
}

void Move_Walk_Exit(World *world, int id, ScMove3 *move, ScCmd *cmd)
{
}

bool Move_Walk_CanExit(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 next)
{
    return true;
}
bool Move_Walk_CanEnter(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 last)
{
    if (move->groundtime > 0 && glms_vec3_norm2(cmd->wishdir) > 0.0f)
        return true;
    return false;
}
