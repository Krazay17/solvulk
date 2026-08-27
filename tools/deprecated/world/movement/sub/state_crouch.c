#include "movement/si_movement.h"
#include "world.h"
#include "sol_math.h"
#include "xform/s_xform.h"
#include "model/s_model.h"
#include "physx/s_body.h"
#include "controller/s_controller.h"

static bool CanSlide(World *world, int id)
{
    CompMovement  *move = &world->movements[id];
    MoveStateData *data = &move->stateData[move->state];

    return false;
}

static bool LeaveState(World *world, int id)
{
    if (Sol_Movement_SetState(world, id, MOVE_SLIDE))
        return true;
    if (!(Sol_Controller_Get(world, id)->actionState & BITC(ACTION_CROUCH)))
        if (Sol_Movement_SetState(world, id, MOVE_IDLE))
            return true;
    if (world->movements[id].wantsJump)
        if (Sol_Movement_SetState(world, id, MOVE_JUMP))
            return true;
    if (Sol_Movement_GetAirtime(world, id) > 0)
        if (Sol_Movement_SetState(world, id, MOVE_FALL))
            return true;
    return false;
}

void Crouch_State_Update(World *world, int id, float dt)
{
    if (LeaveState(world, id))
        return;

    CompMovement  *move = &world->movements[id];
    MoveStateData *data = &move->stateData[move->state];

    float    x    = Sol_Controller_Get(world, id)->wishdir.x;
    float    z    = Sol_Controller_Get(world, id)->wishdir.z;
    vec3s    rot  = Sol_RotFromQuat(world->xforms[id].quat);
    data->as.crouch.strafe = Sol_GetStrafedir(x, z, rot.x, rot.z);
}

void Crouch_State_Enter(World *world, int id)
{
    if (LeaveState(world, id))
        return;

    CompMovement  *move = &world->movements[id];
    MoveStateData *data = &move->stateData[move->state];
    move->targetHeight  = move->baseHeight * 0.7f;
}

void Crouch_State_Exit(World *world, int id)
{

    CompMovement  *move = &world->movements[id];
    MoveStateData *data = &move->stateData[move->state];

    move->targetHeight = move->baseHeight;
}

bool Crouch_State_CanExit(World *world, int id, u32 nextState)
{
    CompMovement *move   = &world->movements[id];
    SolRayResult  result = Sol_RaycastD(
        world, (SolRay){.pos = Sol_Xform_GetPos(world, id), .dir = WORLD_UP, .dist = move->baseHeight * 0.6f}, 0.2f);

    return !result.hit;
}

bool Crouch_State_CanEnter(World *world, int id, u32 lastState, u32 nextState, int slot)
{
    return true;
}
