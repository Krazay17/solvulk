#include "s_movement.h"
#include "world.h"

const MoveStateForce MOVE_STATE_FORCES[MOVEMENTKIND_COUNT][MOVE_STATE_COUNT] =
    {
        [MOVEMENTKIND_PLAYER] =
            {
                [MOVE_IDLE]     = {.speed = 0, .accell = 0, .friction = 10.0f, .gravity = -13.0f},
                [MOVE_WALK]     = {.speed = 7.0f, .accell = 12.0f, .friction = 10.0f, .gravity = -13.0f},
                [MOVE_CROUCH]   = {.speed = 4.0f, .accell = 20.0f, .friction = 10.0f, .gravity = -13.0f},
                [MOVE_FALL]     = {.speed = 6.0f, .accell = 2.0f, .friction = 0.1f, .gravity = -13.0f},
                [MOVE_JUMP]     = {.speed = 6.0f, .accell = 3.0f, .friction = 0.1f, .gravity = -13.0f},
                [MOVE_WALLJUMP] = {.speed = 6.0f, .accell = 2.0f, .friction = 0.5f, .gravity = -13.0f},
                [MOVE_WALLRUN]  = {.speed = 12.0f, .accell = 1.0f, .friction = 1.0f, .gravity = -2.0f},
                [MOVE_MANTLE]   = {.speed = 6.0f, .accell = 1.0f, .friction = 0.0f, .gravity = 0.0f},
                [MOVE_SLIDE]    = {.speed = 2.5f, .accell = 1.0f, .friction = 0.66f, .gravity = -13.0f},
                [MOVE_DEAD]     = {.speed = 0, .accell = 0, .friction = 1.0f, .gravity = -13.0f},
                [MOVE_FLY]      = {.speed = 4.0f, .accell = 7.0f, .friction = 1.0f, .gravity = 0},
                [MOVE_STUN]     = {.speed = 0.0f, .accell = 0.0f, .friction = 5.1f, .gravity = -13.0f},

            },
        [MOVEMENTKIND_SPECTATE] =
            {
                [MOVE_FLY] = {.speed = 10.0f, .accell = 7.0f, .friction = 1.0f, .gravity = 0},
            },
        [MOVEMENTKIND_WIZARD] =
            {
                [MOVE_IDLE]    = {.speed = 0, .accell = 0, .friction = 25.0f, .gravity = -13.0f},
                [MOVE_WALK]    = {.speed = 5.0f, .accell = 20.0f, .friction = 8.0f, .gravity = -13.0f},
                [MOVE_CROUCH]  = {.speed = 4.0f, .accell = 15.0f, .friction = 8.0f, .gravity = -13.0f},
                [MOVE_FALL]    = {.speed = 5.0f, .accell = 5.0f, .friction = 0.1f, .gravity = -13.0f},
                [MOVE_WALLRUN] = {.speed = 8.0f, .accell = 1.0f, .friction = 0.1f, .gravity = -2.0f},
                [MOVE_JUMP]    = {.speed = 5.0f, .accell = 10.0f, .friction = 0.1f, .gravity = -13.0f},
                [MOVE_SLIDE]   = {.speed = 3.0f, .accell = 0.1f, .friction = 2.0f, .gravity = -13.0f},
                [MOVE_DEAD]    = {.speed = 0, .accell = 0, .friction = 1.0f, .gravity = -13.0f},
                [MOVE_FLY]     = {.speed = 5.0f, .accell = 3.5f, .friction = 1.0f, .gravity = 0},
                [MOVE_STUN]    = {.speed = 0.0f, .accell = 0.0f, .friction = 5.1f, .gravity = -13.0f},
            },
};

void Movement3d_Step(World *world, double dt, double time)
{
    float fdt = (float)dt;

    SparseSet_SolMovement *set = Sol_Comp_Set(world, SolMovement);
    for (int i = 0; i < set->cnt; i++)
    {
        int            id         = set->dense[i];
        SolMovement   *movement   = &set->data[i];
        SolController *controller = Sol_Comp_Get(world, id, SolController);
        SolBody3      *body3      = Sol_Comp_Get(world, id, SolBody3);

        const MoveStateForce *forces     = &MOVE_STATE_FORCES[movement->kind][movement->state];
        bool                  isJumpDown = controller->actionState & BITC(ACTION_JUMP);
        vec3s                 vel        = body3->vel;

        if (isJumpDown && !movement->jumpPressedLastFrame)
            movement->wantsJump = true;
        else if (!isJumpDown)
            movement->wantsJump = false;
        movement->jumpPressedLastFrame = isJumpDown;

        movement->stateData[movement->state].elapsed += dt;
        if (MOVE_STATE_FUNCS[movement->state].update)
            MOVE_STATE_FUNCS[movement->state].update(world, id, dt);

        float finalSpeed    = forces->speed * movement->speedMod;
        float finalFriction = forces->friction * movement->frictionMod;
        body3->gravity.y    = forces->gravity * movement->gravityMod;

        vec3s wishdir = controller->wishdir;

        switch (movement->state)
        {
        case MOVE_STUN:
            body3->gravity.y *= 1.33f;
            vel = ApplyFriction3(wishdir, vel, finalFriction, fdt);
            Sol_Physx_SetVel(world, id, vel);
            break;
        case MOVE_IDLE:
        case MOVE_WALK:
            vec3s slopeDir = ProjectOntoGround(world, id, wishdir);
            vel            = ApplyFriction3(slopeDir, vel, finalFriction, fdt);
            vel            = ApplyAccel3(slopeDir, vel, finalSpeed, forces->accell, fdt);
            Sol_Physx_SetVel(world, id, vel);
            break;
        case MOVE_FLY:
            wishdir = controller->wishdirY;
            vel     = ApplyFriction3(wishdir, vel, finalFriction, fdt);
            vel     = ApplyAccel3(wishdir, vel, finalSpeed, forces->accell, fdt);
            Sol_Physx_SetVel(world, id, vel);
        default:
            if (vel.y < 0)
                body3->gravity.y *= 1.33f;
            vel = ApplyFriction3(wishdir, vel, finalFriction, fdt);
            vel = ApplyAccel3(wishdir, vel, finalSpeed, forces->accell, fdt);
            Sol_Physx_SetVellat(world, id, vel);
        }

        movement->lastMoveDir = wishdir;

        if (movement->state != MOVE_JUMP)
            CheckGround(world, id, fdt, movement);

        Knockback(world, id, movement, fdt);
        RestoreFriction(world, id, movement, fdt);
        CrouchHeight(world, id, movement, fdt);
    }
}

void Knockback(World *world, int id, SolMovement *move, float fdt)
{
    if (move->knockDur > 0)
    {
        move->knockDur -= fdt;
        Sol_Physx_LerpVel(world, id, move->knockVel, 0.5f);
    }
}

void RestoreFriction(World *world, int id, SolMovement *move, float fdt)
{
    if (move->frictionMod != 1.0f)
    {
        move->frictionMod = Sol_Math_Lerp(move->frictionMod, 1.0f, 5.0f * fdt);
    }
}

void CrouchHeight(World *world, int id, SolMovement *move, float fdt)
{
    // float currentHeight = Sol_Physx_GetHeight(world, id);
    // float difference    = fabs(currentHeight - move->targetHeight);
    // if (difference < 0.001f)
    //     return;
    // float newHeight = Sol_Math_Lerp(currentHeight, move->targetHeight, 5.0f * fdt);
    // if (newHeight > currentHeight)
    // {
    //     SolRayResult result = Sol_RaycastD(
    //         world, (SolRay){.pos = Sol_Xform_GetPos(world, id), .dir = WORLD_UP, .dist = newHeight * 0.6f}, 0.2f);
    //     if (result.hit)
    //         return;
    // }

    // Sol_Physx_SetHeight(world, id, newHeight);
    // Sol_Model_Get(world, id)->yOffset = newHeight * -0.5f;
}

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