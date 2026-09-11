#include "move3/s_move3.h"
#include "world.h"
#include "sol_math.h"

void Move_Crouch_Update(World *world, int id, ScMove3 *move, ScCmd *cmd, float dt)
{
    MoveStateData *data  = &move->stateData[move->state];

    if (cmd)
    {
        float x                = cmd->wishdir.x;
        float z                = cmd->wishdir.z;
        vec3s rot              = Sol_RotFromQuat(world->xform.rot[id]);
        data->as.crouch.strafe = Sol_GetStrafedir(x, z, rot.x, rot.z);
    }
}

void Move_Crouch_Enter(World *world, int id, ScMove3 *move, ScCmd *cmd)
{
    MoveStateData *data = &move->stateData[move->state];
    move->targetHeight  = move->baseHeight * 0.7f;
}

void Move_Crouch_Exit(World *world, int id, ScMove3 *move, ScCmd *cmd)
{
    MoveStateData *data = &move->stateData[move->state];

    move->targetHeight = move->baseHeight;
}

bool Move_Crouch_CanExit(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 next)
{
    bool     hit   = Sol_Raycast1(
        world, (SolRay){ .start = world->xform.pos[id], .dir = WORLD_UP, .dist = move->baseHeight * 0.6f, .ignoreEnt = id },
        NULL);

    if (hit)
        return false;

    if (!(cmd->actionState & BITC(ACTION_CROUCH)))
        return true;

    return true;
}

bool Move_Crouch_CanEnter(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 last)
{
    if (move->groundtime > 0 && cmd->actionState & BITC(ACTION_CROUCH))
        return true;
    return false;
}
