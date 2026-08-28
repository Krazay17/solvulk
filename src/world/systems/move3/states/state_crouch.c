#include "move3/s_move3.h"
#include "world.h"
#include "sol_math.h"

static bool LeaveState(World *world, int id, SolMove3 *move, SolController *cont)
{
    if (Sol_Movement_SetState(world, id, MOVE_SLIDE))
        return true;
    if (!(cont->actionState & BITC(ACTION_CROUCH)))
        if (Sol_Movement_SetState(world, id, MOVE_IDLE))
            return true;
    if (move->wantsJump)
        if (Sol_Movement_SetState(world, id, MOVE_JUMP))
            return true;
    if (Sol_Movement_GetAirtime(world, id) > 0)
        if (Sol_Movement_SetState(world, id, MOVE_FALL))
            return true;
    return false;
}

void Crouch_State_Update(World *world, int id, float dt)
{
    SolMove3   *move = Sol_Comp_Get(world, id, SolMove3);
    SolController *cont = Sol_Comp_Get(world, id, SolController);
    if (LeaveState(world, id, move, cont))
        return;

    MoveStateData *data  = &move->stateData[move->state];
    SolXform      *xform = Sol_Comp_Get(world, id, SolXform);

    if (cont)
    {
        float x                = cont->wishdir.x;
        float z                = cont->wishdir.z;
        vec3s rot              = Sol_RotFromQuat(xform->rot);
        data->as.crouch.strafe = Sol_GetStrafedir(x, z, rot.x, rot.z);
    }
}

void Crouch_State_Enter(World *world, int id)
{
    SolMove3   *move = Sol_Comp_Get(world, id, SolMove3);
    SolController *cont = Sol_Comp_Get(world, id, SolController);
    if (LeaveState(world, id, move, cont))
        return;

    MoveStateData *data = &move->stateData[move->state];
    move->targetHeight  = move->baseHeight * 0.7f;
}

void Crouch_State_Exit(World *world, int id)
{

    SolMove3   *move = Sol_Comp_Get(world, id, SolMove3);
    MoveStateData *data = &move->stateData[move->state];

    move->targetHeight = move->baseHeight;
}

bool Crouch_State_CanExit(World *world, int id, u32 nextState)
{
    SolMove3 *move = Sol_Comp_Get(world, id, SolMove3);
    // SolRayResult result = Sol_RaycastD(
    //     world, (SolRay){.pos = Sol_Xform_GetPos(world, id), .dir = WORLD_UP, .dist = move->baseHeight * 0.6f}, 0.2f);

    // return !result.hit;
    return true;
}

bool Crouch_State_CanEnter(World *world, int id, u32 lastState, u32 nextState, int slot)
{
    return true;
}
