#include "move3/s_move3.h"
#include "world.h"
#include "sol_math.h"

void Move_Run_Update(World *world, int id, ScMove3 *move, ScCmd *cmd, float dt)
{
    float x                                   = cmd->wishdir.x;
    float z                                   = cmd->wishdir.z;
    vec3s rot                                 = Sol_RotFromQuat(world->xform.rot[id]);
    move->stateData[MOVE_RUN].as.walk.strafe = Sol_GetStrafedir(x, z, rot.x, rot.z);
}

void Move_Run_Enter(World *world, int id, ScMove3 *move, ScCmd *cmd)
{
}

void Move_Run_Exit(World *world, int id, ScMove3 *move, ScCmd *cmd)
{
}

bool Move_Run_CanExit(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 next)
{
    return true;
}
bool Move_Run_CanEnter(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 last)
{
    if (move->groundtime > 0 && glms_vec3_norm2(cmd->wishdir) > 0.0f)
        return true;
    return false;
}

const MoveStateFuncs move_run_funcs = {
    .update   = Move_Run_Update,
    .enter    = Move_Run_Enter,
    .exit     = Move_Run_Exit,
    .canExit  = Move_Run_CanExit,
    .canEnter = Move_Run_CanEnter,
};
