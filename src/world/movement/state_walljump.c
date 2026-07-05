#include "movement_i.h"
#include "world.h"
#include "sol_math.h"
#include "xform/s_xform.h"
#include "model/s_model.h"
#include "physx/s_body.h"
#include "controller/s_controller.h"

#define DASH_VEL 13.0f
#define DASH_DURATION 0.4f
#define DAMPING 4.0f

void Walljump_State_Update(World *world, int id, float dt)
{
    CompMovement  *move = &world->movements[id];
    MoveStateData *data = &move->stateData[MOVE_WALLJUMP];
    data->elapsed += dt;
    float alpha = 1.0f - (data->elapsed / DASH_DURATION);

    if (data->elapsed >= DASH_DURATION)
    {
        Sol_Movement_SetState(world, id, MOVE_IDLE);
        return;
    }

    vec3s vel = Sol_Physx_GetVel(world, id);
    vel       = Sol_Math_DampDir(vel, WORLD_UP, alpha, DAMPING, dt);
    // vel       = Sol_Math_DampDir(vel, data->dir, alpha, DAMPING, dt);
    Sol_Physx_SetVel(world, id, vel);
}

void Walljump_State_Enter(World *world, int id)
{
    CompMovement  *move     = &world->movements[id];
    MoveStateData *data     = &move->stateData[MOVE_WALLJUMP];
    vec3s          vel      = Sol_Physx_GetVel(world, id);
    vec3s          up2      = {0.0f, 1.8f, 0.0f};
    vec3s          finalDir = vecAdd(move->wallNormal, up2);
    finalDir                = vecAdd(finalDir, vecNorm(vel));
    data->dir               = vecNorm(finalDir);

    vec3s finalVel    = vecSca(data->dir, DASH_VEL);
    float targetUpVel = finalVel.y;
    if (vel.y < targetUpVel)
        finalVel.y = targetUpVel - vel.y;
    else
        finalVel.y = 0;

    Sol_Physx_AddVel(world, id, finalVel);

    vec3s dirToWall = glms_vec3_sub(Sol_Xform_GetPos(world, id), move->lastTouch);
    dirToWall       = glms_vec3_normalize(dirToWall);
    float x = dirToWall.x;
    float z = dirToWall.z;
    vec3s    rot    = Sol_RotFromQuat(world->xforms[id].quat);
    AnimDesc desc   = {.anim = ANIM_WALLJUMP_LEFT, .layerId = ANIM_LAYER_BASE, .blendIn = 0.05f, .playKind = ANIMPLAYKIND_ONESHOT};
    switch (Sol_GetStrafedir(x, z, rot.x, rot.z))
    {
    case STRAFE_LEFT:
    case STRAFE_FWD_LEFT:
    case STRAFE_BWD_LEFT:
        desc.anim = ANIM_WALLJUMP_LEFT;
        Sol_Model_PlayAnim(world, id, desc);
        break;
    case STRAFE_RIGHT:
    case STRAFE_BWD_RIGHT:
    case STRAFE_FWD_RIGHT:
        desc.anim = ANIM_WALLJUMP_RIGHT;
        Sol_Model_PlayAnim(world, id, desc);
        break;
    default:
        desc.anim = ANIM_WALLJUMP_LEFT;
        Sol_Model_PlayAnim(world, id, desc);
        break;
    }
}

void Walljump_State_Exit(World *world, int id)
{
}

bool Walljump_State_CanExit(World *world, int id, u32 nextState)
{
    return nextState != MOVE_WALLJUMP;
}

bool Walljump_State_CanEnter(World *world, int id, u32 lastState, u32 nextState, int slot)
{
    return true;
}
