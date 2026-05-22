#include "sol_core.h"

#include "movement_i.h"

static bool LeaveState(World *world, int id)
{
    CompXform *xform  = &world->xforms[id];
    bool       onWall = false;
    for (int j = 1; j < 9; j++)
    {
        vec3s        rotated_offset = glms_quat_rotatev(xform->quat, VECTOR_RADIAL_DIRECTIONS[j]);
        SolRayResult result         = Sol_RaycastD(
            world, (SolRay){.dist = Sol_Physx_GetDims(world, id).x * 2.0f, .dir = rotated_offset, .pos = xform->pos},
            1.0f);
        if (result.hit)
            onWall = true;
    }

    if (!onWall)
        if (Sol_Movement_SetState(world, id, MOVE_IDLE))
            return true;
    if (!Sol_Controller_IsActionState(world, id, ACTION_JUMP))
        if (Sol_Movement_SetState(world, id, MOVE_IDLE))
            return true;
    if (Sol_Physx_GetGrounded(world, id))
        if (Sol_Movement_SetState(world, id, MOVE_IDLE))
            return true;
    return false;
}

void Wallrun_State_Update(World *world, int id, float dt)
{
    if (LeaveState(world, id))
        return;

    CompMovement *movement = &world->movements[id];

    float    x    = Sol_Controller_GetWishdir(world, id).x;
    float    z    = Sol_Controller_GetWishdir(world, id).z;
    vec3s    rot  = Sol_RotFromQuat(world->xforms[id].quat);
    AnimDesc desc = {.layerId = ANIM_LAYER_BASE};
    switch (Sol_GetStrafedir(x, z, rot.x, rot.z))
    {
    case STRAFE_FWD:
        desc.anim = ANIM_WALK_FWD;
        Sol_Model_PlayAnim(world, id, desc);
        break;
    case STRAFE_BWD:
    case STRAFE_BWD_RIGHT:
    case STRAFE_BWD_LEFT:
        desc.anim = ANIM_WALK_BWD;
        Sol_Model_PlayAnim(world, id, desc);
        break;
    case STRAFE_LEFT:
    case STRAFE_FWD_LEFT:
        desc.anim = ANIM_WALK_LEFT;
        Sol_Model_PlayAnim(world, id, desc);
        break;
    case STRAFE_RIGHT:
    case STRAFE_FWD_RIGHT:
        desc.anim = ANIM_WALK_RIGHT;
        Sol_Model_PlayAnim(world, id, desc);
        break;
    default:
        desc.anim = ANIM_WALK_FWD;
        Sol_Model_PlayAnim(world, id, desc);
        break;
    }
    const MoveStateForce *forces   = &MOVE_STATE_FORCES[movement->configId][movement->moveState];
    float                 speedDif = Sol_Physx_GetSpeed(world, id) / forces->speed;
    Sol_Model_SetAnimSpeed(world, id, ANIM_LAYER_BASE, speedDif);

    CompXform   *xform  = &world->xforms[id];
    SolRayResult result = {0};
    for (int j = 1; j < 9; j++)
    {
        vec3s rotated_offset = glms_quat_rotatev(xform->quat, VECTOR_RADIAL_DIRECTIONS[j]);
        result               = Sol_RaycastD(
            world, (SolRay){.dist = Sol_Physx_GetDims(world, id).x * 2.0f, .dir = rotated_offset, .pos = xform->pos},
            1.0f);
        if (result.hit)
            break;
    }

    if (!result.hit)
        return;

    vec3s wishdir = Sol_Controller_GetWishdir(world, id);

    // 2. Find how much of wishdir is pushing directly into the wall
    float push_into_wall = glms_vec3_dot(wishdir, result.norm);
    if (push_into_wall > 0)
        if (Sol_Movement_SetState(world, id, MOVE_IDLE))
            return;

    // 3. Subtract that inward push from the original wishdir to get the plane tangent
    vec3s project = glms_vec3_sub(wishdir, glms_vec3_scale(result.norm, push_into_wall));

    // 4. Normalize to get a pure directional vector along the wall
    // (Check for zero length in case wishdir was perfectly perpendicular to the wall)
    if (glms_vec3_norm2(project) > 0.001f)
    {
        project = glms_vec3_normalize(project);
    }

    Sol_Physx_SetVel(world, id, glms_vec3_scale(project, 6.0f));
}

void Wallrun_State_Enter(World *world, int id)
{
    if (LeaveState(world, id))
        return;
}

void Wallrun_State_Exit(World *world, int id)
{
    Sol_Physx_Impulse(world, id, vecSca(WORLD_UP, 100.0f));
}

bool Wallrun_State_CanExit(World *world, int id, u32 nextState)
{
    return true;
}

bool Wallrun_State_CanEnter(World *world, int id, u32 lastState, u32 nextState)
{
    if (lastState == MOVE_WALLRUN)
        return false;

    CompXform *xform = &world->xforms[id];
    for (int j = 1; j < 9; j++)
    {
        vec3s        rotated_offset = glms_quat_rotatev(xform->quat, VECTOR_RADIAL_DIRECTIONS[j]);
        SolRayResult result         = Sol_RaycastD(
            world, (SolRay){.dist = Sol_Physx_GetDims(world, id).x * 2.0f, .dir = rotated_offset, .pos = xform->pos},
            1.0f);
        if (result.hit)
            return true;
    }

    return false;
}
