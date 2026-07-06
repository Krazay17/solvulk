#include "movement_i.h"
#include "sol_core.h"
#include "world.h"
#include "sol_math.h"
#include "xform/s_xform.h"
#include "model/s_model.h"
#include "physx/s_body.h"
#include "controller/s_controller.h"

#define MIN_WALL_ANGLE -0.7f
#define MAX_WALL_ANGLE 0.7f
#define COYOTE_TIMER 0.15f
#define BOOST_TIMEOUT 2.0f
#define BOOST_AMOUNT 9.0f
#define DISTANCE_CHECK 0.15f

static bool CheckWall(World *world, int id, SolRayResult *result, float addRadius)
{
    CompXform *xform  = &world->xforms[id];
    vec3s      pos    = xform->pos;
    vec3s      dims   = Sol_Physx_GetDims(world, id);
    float      radius = dims.x + addRadius;
    for (int i = -1; i < 2; i++)
    {
        for (int j = 1; j < 9; j++)
        {
            vec3s finalPos = pos;
            finalPos.y += (float)i * (dims.y * 0.4f);
            vec3s rotated_offset = glms_quat_rotatev(xform->quat, VECTOR_RADIAL_DIRECTIONS[j]);
            *result              = Sol_Raycast(world, (SolRay){.dist = radius, .dir = rotated_offset, .pos = finalPos});
            float dot            = glms_vec3_dot(result->norm, WORLD_UP);
            if (result->hit && dot > MIN_WALL_ANGLE && dot < MAX_WALL_ANGLE)
            {
                world->movements[id].lastTouch = result->norm;
                return true;
            }
        }
    }

    return false;
}

static bool LeaveState(World *world, int id)
{
    CompMovement *move = &world->movements[id];
    if (Sol_Controller_IsActionState(world, id, ACTION_CROUCH) || Sol_Movement_GetGroundtime(world, id) > COYOTE_TIMER)
        if (Sol_Movement_SetState(world, id, MOVE_IDLE))
            return true;
    if (!Sol_Controller_IsActionState(world, id, ACTION_JUMP))
        if (Sol_Movement_SetState(world, id, MOVE_WALLJUMP))
            return true;
    if (Sol_Movement_SetState(world, id, MOVE_MANTLE))
        return true;
    return false;
}

void RunVel(World *world, int id, float boost)
{
    CompMovement  *movement   = &world->movements[id];
    MoveStateData *data       = &movement->stateData[MOVE_WALLRUN];
    vec3s          prevvel    = Sol_Physx_GetVel(world, id);
    vec3s          prevLatVel = prevvel;
    prevLatVel.y              = 0;
    float targetSpeed         = fmaxf(glms_vec3_norm(prevLatVel), boost);

    vec3s project;
    vec3s targetVel;
    vec3s lookdir      = world->controllers[id].lookdir;
    vec3s wishdir      = Sol_Controller_GetWishdir(world, id);
    lookdir.y          = 0;
    lookdir            = vecNorm(lookdir);
    float lookIntoWall = -glms_vec3_dot(lookdir, movement->wallNormal);

    if (lookIntoWall > 0.7f)
    {
        project   = glms_vec3_sub(lookdir, glms_vec3_scale(movement->wallNormal, -lookIntoWall));
        project.y = lookIntoWall;
        project   = glms_vec3_normalize(project);
        targetVel = glms_vec3_scale(project, targetSpeed);
    }
    else
    {
        float push_into_wall = glms_vec3_dot(prevLatVel, movement->wallNormal);
        project              = glms_vec3_sub(prevLatVel, glms_vec3_scale(movement->wallNormal, push_into_wall));
        project              = glms_vec3_normalize(project);
        targetVel            = glms_vec3_scale(project, targetSpeed);
        targetVel.y          = prevvel.y;
    }

    Sol_Physx_SetVel(world, id, targetVel);
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
    bool         goodWall = CheckWall(world, id, &result, DISTANCE_CHECK + 0.1f);
    if (goodWall)
    {
        data->accum          = 0;
        movement->wallNormal = result.norm;
        movement->lastTouch  = result.pos;
    }
    else if (!goodWall && data->accum >= COYOTE_TIMER)
    {
        Sol_Movement_SetState(world, id, MOVE_IDLE);
        return;
    }

    RunVel(world, id, Sol_Math_Lerp(BOOST_AMOUNT, 0.0f, data->elapsed / BOOST_TIMEOUT));

    vec3s dirToWall    = glms_vec3_sub(xform->pos, movement->lastTouch);
    dirToWall          = glms_vec3_normalize(dirToWall);
    float velToWallDot = -vecDot(Sol_Physx_GetVel(world, id), vecCrs(dirToWall, WORLD_UP));
    sollog(velToWallDot);

    // ANIMATION
    float    x        = dirToWall.x;
    float    z        = dirToWall.z;
    vec3s    rot      = Sol_RotFromQuat(world->xforms[id].quat);
    AnimDesc desc     = {.layerId = ANIM_LAYER_BASE};
    float    speedMod = 1.0f;
    switch (Sol_GetStrafedir(x, z, rot.x, rot.z))
    {
    case STRAFE_FWD:
        desc.anim = ANIM_WALK_FWD;
        Sol_Model_PlayAnim(world, id, desc);
        break;
    case STRAFE_BWD:
        desc.anim = ANIM_WALLRUN_FWD;
        // speedMod = -1.0f;
        Sol_Model_PlayAnim(world, id, desc);
        break;
    case STRAFE_LEFT:
    case STRAFE_FWD_LEFT:
    case STRAFE_BWD_LEFT:
        desc.anim = ANIM_WALLRUN_RIGHT;
        speedMod  = velToWallDot < 0 ? 1.5f : -1.5f;
        Sol_Model_PlayAnim(world, id, desc);
        break;
    case STRAFE_RIGHT:
    case STRAFE_BWD_RIGHT:
    case STRAFE_FWD_RIGHT:
        desc.anim = ANIM_WALLRUN_LEFT;
        speedMod  = velToWallDot > 0 ? 1.5f : -1.5f;
        Sol_Model_PlayAnim(world, id, desc);
        break;
    default:
        desc.anim = ANIM_WALK_FWD;
        Sol_Model_PlayAnim(world, id, desc);
        break;
    }
    const MoveStateForce *forces   = &MOVE_STATE_FORCES[movement->kind][movement->state];
    float                 speedDif = Sol_Physx_GetSpeed(world, id) / forces->speed;
    Sol_Model_SetAnimSpeed(world, id, ANIM_LAYER_BASE, speedDif * speedMod);
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
    RunVel(world, id, BOOST_AMOUNT);
    data->accum = 0;
}

void Wallrun_State_Exit(World *world, int id)
{
    CompMovement  *movement = &world->movements[id];
    MoveStateData *data     = &movement->stateData[MOVE_WALLRUN];

    data->lastExited = solState.gameTime;
}

bool Wallrun_State_CanExit(World *world, int id, u32 nextState)
{
    return nextState != MOVE_WALLRUN;
}

bool Wallrun_State_CanEnter(World *world, int id, u32 lastState, u32 nextState, int slot)
{
    if (Sol_Controller_IsActionState(world, id, ACTION_CROUCH))
        return false;

    SolRayResult result   = {0};
    bool         goodWall = CheckWall(world, id, &result, DISTANCE_CHECK);
    if (goodWall)
    {
        CompMovement *move = &world->movements[id];
        move->lastTouch    = result.pos;
        move->wallNormal   = result.norm;
    }

    return goodWall;
}
