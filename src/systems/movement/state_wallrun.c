#include "sol_core.h"

#include "movement_i.h"

#define MIN_WALL_ANGLE -0.7f
#define MAX_WALL_ANGLE 0.7f
#define COYOTE_TIMER 0.2f

static bool CheckWall(World *world, int id, SolRayResult *result)
{
    CompXform *xform  = &world->xforms[id];
    vec3s      pos    = xform->pos;
    vec3s      dims   = Sol_Physx_GetDims(world, id);
    float      radius = dims.x * 2.0f;
    for (int i = -1; i < 1; i++)
    {
        for (int j = 1; j < 9; j++)
        {
            vec3s finalPos = pos;
            finalPos.y += (float)i * (dims.y * 0.35f);
            vec3s rotated_offset = glms_quat_rotatev(xform->quat, VECTOR_RADIAL_DIRECTIONS[j]);
            *result              = Sol_Raycast(world, (SolRay){.dist = radius, .dir = rotated_offset, .pos = finalPos});
            float dot            = glms_vec3_dot(result->norm, WORLD_UP);
            printf("WallNorm: %f\n", dot);
            if (result->hit && dot > MIN_WALL_ANGLE && dot < MAX_WALL_ANGLE)
                return true;
        }
    }
    return false;
}

static bool LeaveState(World *world, int id)
{
    if (!Sol_Controller_IsActionState(world, id, ACTION_JUMP))
        if (Sol_Movement_SetState(world, id, MOVE_WALLJUMP))
            return true;
    if (Sol_Physx_GetGroundtime(world, id) > COYOTE_TIMER)
        if (Sol_Movement_SetState(world, id, MOVE_IDLE))
            return true;
    return false;
}

void Wallrun_State_Update(World *world, int id, float dt)
{
    if (LeaveState(world, id))
        return;

    CompXform     *xform    = &world->xforms[id];
    CompMovement  *movement = &world->movements[id];
    MoveStateData *data     = &movement->stateData[MOVE_WALLRUN];
    data->elapsed += dt;
    data->accum += dt;

    SolRayResult result   = {0};
    bool         goodWall = CheckWall(world, id, &result);
    if (goodWall)
    {
        data->accum = 0;
        movement->wallNormal = result.norm;
        movement->lastTouch  = result.pos;
    }
    else if (!goodWall && data->accum >= COYOTE_TIMER)
    {
        Sol_Movement_SetState(world, id, MOVE_IDLE);
        return;
    }

    movement->wallDot = vecDot(movement->wallNormal, Sol_Cam_GetRight());
    vec3s prevVel     = Sol_Physx_GetVel(world, id);
    vec3s wishdir     = prevVel; // Sol_Controller_GetWishdir(world, id);

    float push_into_wall = glms_vec3_dot(wishdir, movement->wallNormal);

    // 3. Subtract that inward push from the original wishdir to get the plane tangent
    vec3s project = glms_vec3_sub(wishdir, glms_vec3_scale(movement->wallNormal, push_into_wall));

    // 4. Normalize to get a pure directional vector along the wall
    // (Check for zero length in case wishdir was perfectly perpendicular to the wall)
    if (glms_vec3_norm2(project) > 0.001f)
    {
        project = glms_vec3_normalize(project);
        Sol_Physx_SetVel(world, id,
                         glms_vec3_lerp(prevVel, glms_vec3_scale(project, Sol_Physx_GetSpeed(world, id)), 0.33f));
    }

    vec3s dirToWall = glms_vec3_sub(xform->pos, movement->lastTouch);
    dirToWall       = glms_vec3_normalize(dirToWall);

    // ANIMATION
    float    x           = dirToWall.x;
    float    z           = dirToWall.z;
    vec3s    rot         = Sol_RotFromQuat(world->xforms[id].quat);
    AnimDesc desc        = {.layerId = ANIM_LAYER_BASE};
    float    animReverse = 1.0f;
    switch (Sol_GetStrafedir(x, z, rot.x, rot.z))
    {
    case STRAFE_FWD:
        desc.anim = ANIM_WALK_FWD;
        Sol_Model_PlayAnim(world, id, desc);
        break;
    case STRAFE_BWD:
        desc.anim   = ANIM_WALK_BWD;
        animReverse = -1.0f;
        Sol_Model_PlayAnim(world, id, desc);
        break;
    case STRAFE_LEFT:
    case STRAFE_FWD_LEFT:
    case STRAFE_BWD_LEFT:
        desc.anim = ANIM_WALK_LEFT;
        Sol_Model_PlayAnim(world, id, desc);
        break;
    case STRAFE_RIGHT:
    case STRAFE_BWD_RIGHT:
    case STRAFE_FWD_RIGHT:
        desc.anim = ANIM_WALK_RIGHT;
        Sol_Model_PlayAnim(world, id, desc);
        break;
    default:
        desc.anim = ANIM_WALK_FWD;
        Sol_Model_PlayAnim(world, id, desc);
        break;
    }
    const MoveStateForce *forces   = &MOVE_STATE_FORCES[movement->kind][movement->state];
    float                 speedDif = Sol_Physx_GetSpeed(world, id) / forces->speed;
    Sol_Model_SetAnimSpeed(world, id, ANIM_LAYER_BASE, speedDif * animReverse);
}

void Wallrun_State_Enter(World *world, int id)
{
    if (LeaveState(world, id))
        return;
    CompMovement  *movement = &world->movements[id];
    MoveStateData *data     = &movement->stateData[MOVE_WALLRUN];
    if (Sol_Physx_GetVel(world, id).y < 0.0f)
        Sol_Physx_SetVelY(world, id, 0.0f);

    data->enterVel = Sol_Physx_GetVel(world, id);
    data->accum    = 0;
}

void Wallrun_State_Exit(World *world, int id)
{
    world->movements[id].wallDot = 0;
}

bool Wallrun_State_CanExit(World *world, int id, u32 nextState)
{
    return nextState != MOVE_WALLRUN;
}

bool Wallrun_State_CanEnter(World *world, int id, u32 lastState, u32 nextState)
{
    CompMovement *move = &world->movements[id];
    // MoveStateData *data    = &move->stateData[MOVE_WALLRUN];
    // double         now     = Sol_GetGameTime();
    // double         readyAt = data->lastExited + COOLDOWN;
    // if (now < readyAt)
    //     return false;

    SolRayResult result   = {0};
    bool         goodWall = CheckWall(world, id, &result);
    if (goodWall)
    {
        move->lastTouch  = result.pos;
        move->wallNormal = result.norm;
    }

    return goodWall;
}
