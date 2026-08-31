#include "s_move3.h"
#include "world.h"
#include "sol_core.h"

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

void Move3_Step(World *world, double dt)
{
    float fdt = (float)dt;

    SparseSet_SolMove3 *set = Sol_Comp_Set(world, SolMove3);
    for (int i = 0; i < set->cnt; i++)
    {
        int            id         = set->dense[i];
        SolMove3      *move       = &set->data[i];
        SolController *controller = Sol_Comp_Get(world, id, SolController);
        SolBody3      *body3      = Sol_Comp_Get(world, id, SolBody3);

        const MoveStateForce *forces     = &MOVE_STATE_FORCES[move->kind][move->state];
        bool                  isJumpDown = controller->actionState & BITC(ACTION_JUMP);
        vec3s                 vel        = body3->vel;
        vec3s                 wishdir    = controller->wishdir;

        if (isJumpDown && !move->jumpPressedLastFrame)
            move->wantsJump = true;
        else if (!isJumpDown)
            move->wantsJump = false;
        move->jumpPressedLastFrame = isJumpDown;
        move->lastMoveDir          = wishdir;

        move->stateData[move->state].elapsed += dt;
        if (MOVE_STATE_FUNCS[move->state].update)
            MOVE_STATE_FUNCS[move->state].update(world, id, dt);

        float finalSpeed    = forces->speed;    // * move->speedMod;
        float finalFriction = forces->friction * move->frictionMod;
        body3->gravity.y    = forces->gravity;  // * move->gravityMod;

        switch (move->state)
        {
        case MOVE_STUN:
            body3->gravity.y *= 1.33f;
            vel        = ApplyFriction3(wishdir, vel, finalFriction, fdt);
            body3->vel = vel;
            break;
        case MOVE_IDLE:
        case MOVE_WALK:
            vec3s slopeDir = ProjectOntoGround(WORLD_UP, wishdir);
            vel            = ApplyFriction3(slopeDir, vel, finalFriction, fdt);
            vel            = ApplyAccel3(slopeDir, vel, finalSpeed, forces->accell, fdt);
            body3->vel     = vel;
            break;
        case MOVE_FLY:
            wishdir    = controller->wishdirY;
            vel        = ApplyFriction3(wishdir, vel, finalFriction, fdt);
            vel        = ApplyAccel3(wishdir, vel, finalSpeed, forces->accell, fdt);
            body3->vel = vel;
        default:
            if (vel.y < 0)
                body3->gravity.y *= 1.33f;
            vel          = ApplyFriction3(wishdir, vel, finalFriction, fdt);
            vel          = ApplyAccel3(wishdir, vel, finalSpeed, forces->accell, fdt);
            body3->vel.x = vel.x;
            body3->vel.z = vel.z;
        }

        // if (move->state != MOVE_JUMP)
        //     CheckGround(world, id, fdt, move);

        if (move->knockDur > 0)
        {
            move->knockDur -= fdt;
            body3->vel = glms_vec3_lerp(body3->vel, move->knockVel, 0.5f);
        }
        if (move->frictionMod != 1.0f)
            move->frictionMod = Sol_Math_Lerp(move->frictionMod, 1.0f, 5.0f * fdt);

        // TEMP
        // if (Sol_Comp_Has(world, id, SolXform))
        // {
        //     SolXform *xform = Sol_Comp_Get(world, id, SolXform);
        //     if (xform->pos.y <= 0)
        //     {
        //         move->airtime = 0;
        //         move->groundtime += fdt;
        //     }
        //     else
        //     {
        //         move->groundtime = 0;
        //         move->airtime += fdt;
        //     }
        // }
    }
}

void Move3_Init(World *world)
{
}

void CrouchHeight(World *world, int id, SolMove3 *move, float fdt)
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

bool Sol_Movement_SetState(World *world, int id, MoveState state)
{
    SolMove3        *move     = Sol_Comp_Get(world, id, SolMove3);
    const StateFunc *prevfunc = &MOVE_STATE_FUNCS[move->state];
    const StateFunc *nextfunc = &MOVE_STATE_FUNCS[state];

    if (!nextfunc->canEnter || !prevfunc->canExit)
        return false;
    if (move->state == state)
        return false;
    if (!prevfunc->canExit(world, id, state))
        return false;
    if (!nextfunc->canEnter(world, id, (u32)move->state, (u32)state, 0))
        return false;

    // printf("LastState: %d, CurrentState: %d\n", move->state, state);

    prevfunc->exit(world, id);
    move->stateData[move->state].lastExited = solState.appTime;

    move->state                              = state;
    move->stateData[move->state].lastEntered = solState.appTime;
    move->stateData[move->state].elapsed     = 0.0f;
    nextfunc->enter(world, id);

    // Sol_Physx_SetGrav(world, id, (vec3s){0, -MOVE_STATE_FORCES[move->kind][move->state].gravity, 0});

    return true;
}