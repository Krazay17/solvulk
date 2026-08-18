#include "movement_i.h"
#include "world.h"
#include "sol_core.h"
#include "sol_math.h"
#include "movement/s_movement.h"
#include "model/s_model.h"
#include "physx/s_body.h"

const StateFunc MOVE_STATE_FUNCS[MOVE_STATE_COUNT] = {
    [MOVE_IDLE] =
        {
            Sol_Movement_Idle_Update,
            Sol_Movement_Idle_Enter,
            Sol_Movement_Idle_Exit,
            Sol_Movement_Idle_CanExit,
            Sol_Movement_Idle_CanEnter,
        },
    [MOVE_WALK] =
        {
            Sol_Movement_Walk_Update,
            Sol_Movement_Walk_Enter,
            Sol_Movement_Walk_Exit,
            Sol_Movement_Walk_CanExit,
            Sol_Movement_Walk_CanEnter,
        },
    [MOVE_FALL] =
        {
            Sol_Movement_Fall_Update,
            Sol_Movement_Fall_Enter,
            Sol_Movement_Fall_Exit,
            Sol_Movement_Fall_CanExit,
            Sol_Movement_Fall_CanEnter,
        },
    [MOVE_JUMP] =
        {
            Sol_Movement_Jump_Update,
            Sol_Movement_Jump_Enter,
            Sol_Movement_Jump_Exit,
            Sol_Movement_Jump_CanExit,
            Sol_Movement_Jump_CanEnter,
        },
    [MOVE_FLY] =
        {
            Sol_Movement_Fly_Update,
            Sol_Movement_Fly_Enter,
            Sol_Movement_Fly_Exit,
            Sol_Movement_Fly_CanExit,
            Sol_Movement_Fly_CanEnter,
        },
    [MOVE_CROUCH] =
        {
            Crouch_State_Update,
            Crouch_State_Enter,
            Crouch_State_Exit,
            Crouch_State_CanExit,
            Crouch_State_CanEnter,
        },
    [MOVE_SLIDE] =
        {
            Slide_State_Update,
            Slide_State_Enter,
            Slide_State_Exit,
            Slide_State_CanExit,
            Slide_State_CanEnter,
        },
    [MOVE_WALLRUN] =
        {
            Wallrun_State_Update,
            Wallrun_State_Enter,
            Wallrun_State_Exit,
            Wallrun_State_CanExit,
            Wallrun_State_CanEnter,
        },
    [MOVE_WALLJUMP] =
        {
            Walljump_State_Update,
            Walljump_State_Enter,
            Walljump_State_Exit,
            Walljump_State_CanExit,
            Walljump_State_CanEnter,
        },
    [MOVE_DEAD] =
        {
            Dead_State_Update,
            Dead_State_Enter,
            Dead_State_Exit,
            Dead_State_CanExit,
            Dead_State_CanEnter,
        },
    [MOVE_STUN] =
        {
            Stun_State_Update,
            Stun_State_Enter,
            Stun_State_Exit,
            Stun_State_CanExit,
            Stun_State_CanEnter,
        },
    [MOVE_MANTLE] =
        {
            Mantle_State_Update,
            Mantle_State_Enter,
            Mantle_State_Exit,
            Mantle_State_CanExit,
            Mantle_State_CanEnter,
            Mantle_State_Draw,
        },
};

void CrouchHeight(World *world, int id, float fdt)
{
    CompMovement *move          = &world->movements[id];
    float         currentHeight = Sol_Physx_GetHeight(world, id);
    float         difference    = fabs(currentHeight - move->targetHeight);
    if (difference < 0.001f)
        return;
    float newHeight = Sol_Math_Lerp(currentHeight, move->targetHeight, 5.0f * fdt);
    if (newHeight > currentHeight)
    {
        SolRayResult result =
            Sol_RaycastD(world, (SolRay){.pos = Sol_Xform_GetPos(world, id), .dir = WORLD_UP, .dist = newHeight * 0.6f}, 0.2f);
        if (result.hit)
            return;
    }

    Sol_Physx_SetHeight(world, id, newHeight);
    Sol_Model_SetOffsetY(world, id, newHeight * -0.5f);
}

void Knockback(World *world, int id, float fdt)
{
    CompMovement *move = &world->movements[id];
    if(move->knockDur > 0)
    {
        move->knockDur -= fdt;
        Sol_Physx_LerpVel(world, id, move->knockVel, 0.5f);
    }
}

void RestoreFriction(World *world, int id, CompMovement *move, float fdt)
{
    if (move->frictionMod != 1.0f)
    {
        move->frictionMod = Sol_Math_Lerp(move->frictionMod, 1.0f, 5.0f *fdt);
    }
}

vec3s GroundSlope(World *world, int id)
{
    vec3s normal     = Sol_Physx_GetGround(world, id);
    float dot        = glms_vec3_dot(WORLD_DOWN, normal);
    vec3s projection = glms_vec3_scale(normal, dot);
    vec3s slope      = glms_vec3_sub(WORLD_DOWN, projection);
    return glms_normalize(slope);
}

vec3s ProjectOntoGround(World *world, int id, vec3s wishdir)
{
    vec3s ground = Sol_Physx_GetGround(world, id);
    float dot    = glms_vec3_dot(wishdir, ground);
    // dot = fmaxf(-0.5f, fminf(0.5, dot));
    return glms_vec3_sub(wishdir, glms_vec3_scale(ground, dot));
}