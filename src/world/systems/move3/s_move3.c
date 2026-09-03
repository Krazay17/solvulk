#include "s_move3.h"
#include "world.h"
#include "sol_core.h"

#include <omp.h>

#define MAX_WALK_ANGLE 0.7f

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
    int i;

    SparseSet_ScMove3 *set = Sol_Comp_Set(world, ScMove3);
#pragma omp parallel for schedule(dynamic)
    for ( i = 0; i < set->cnt; i++)
    {
        int      id    = set->dense[i];
        ScMove3 *move  = &set->data[i];
        ScCmd   *cmd   = Sol_Comp_Get(world, id, ScCmd);
        ScBody3 *body3 = Sol_Comp_Get(world, id, ScBody3);

        const MoveStateForce *forces     = &MOVE_STATE_FORCES[move->kind][move->state];
        bool                  isJumpDown = cmd->actionState & BITC(ACTION_JUMP);
        vec3s                 vel        = body3->vel;
        vec3s                 wishdir    = cmd->wishdir;

        GroundCheck(world, id, move, fdt);

        if (isJumpDown && !move->jumpPressedLastFrame)
            move->wantsJump = true;
        else if (!isJumpDown)
            move->wantsJump = false;
        move->jumpPressedLastFrame = isJumpDown;
        move->lastMoveDir          = wishdir;

        move->stateData[move->state].elapsed += dt;
        if (MOVE_STATE_FUNCS[move->state].update)
            MOVE_STATE_FUNCS[move->state].update(world, id, dt);

        float finalSpeed    = forces->speed; // * move->speedMod;
        float finalFriction = forces->friction * move->frictionMod;
        body3->gravity.y    = forces->gravity; // * move->gravityMod;

        switch (move->state)
        {
        case MOVE_MANTLE:
            break;
        case MOVE_STUN:
            body3->gravity.y *= 1.33f;
            vel        = ApplyFriction3(wishdir, vel, finalFriction, fdt);
            body3->vel = vel;
            break;
        case MOVE_IDLE:
        case MOVE_WALK:
        case MOVE_CROUCH:
            vec3s slopeDir = ProjectOntoGround(move->groundNorm, wishdir);
            vel            = ApplyFriction3(slopeDir, vel, finalFriction, fdt);
            vel            = ApplyAccel3(slopeDir, vel, finalSpeed, forces->accell, fdt);
            body3->vel     = vel;
            break;
        case MOVE_FLY:
            wishdir    = cmd->wishdir;
            vel        = ApplyFriction3(wishdir, vel, finalFriction, fdt);
            vel        = ApplyAccel3(wishdir, vel, finalSpeed, forces->accell, fdt);
            body3->vel = vel;
        case MOVE_FALL:
            if (vel.y < 0)
                body3->gravity.y *= 1.33f;
            vel          = ApplyFriction3(wishdir, vel, finalFriction, fdt);
            vel          = ApplyAccel3(wishdir, vel, finalSpeed, forces->accell, fdt);
            body3->vel.x = vel.x;
            body3->vel.z = vel.z;
            break;
            // default:
            // if (vel.y < 0)
            //     body3->gravity.y *= 1.33f;
            // vel        = ApplyFriction3(wishdir, vel, finalFriction, fdt);
            // vel        = ApplyAccel3(wishdir, vel, finalSpeed, forces->accell, fdt);
            // body3->vel = vel;
        }

        if (move->knockDur > 0.0f)
        {
            move->knockDur -= fdt;
            const float knockFactor = 1.0f - expf(-10.0f * fdt); // Decays smoothly over time
            body3->vel              = glms_vec3_lerp(body3->vel, move->knockVel, knockFactor);
        }

        // Framerate-independent friction modifier recovery
        if (move->frictionMod != 1.0f)
        {
            const float fricFactor = 1.0f - expf(-5.0f * fdt);
            move->frictionMod      = Sol_Math_Lerp(move->frictionMod, 1.0f, fricFactor);
        }
    }
}

void Move3_Init(World *world)
{
}

void CrouchHeight(World *world, int id, ScMove3 *move, float fdt)
{
    ScBody3 *body          = Sol_Comp_Get(world, id, ScBody3);
    float    currentHeight = body->dims.y;
    float    difference    = fabs(currentHeight - move->targetHeight);
    if (difference < 0.001f)
        return;
    float newHeight = Sol_Math_Lerp(currentHeight, move->targetHeight, 5.0f * fdt);
    if (newHeight > currentHeight)
    {
        if (Sol_Raycast1D(
                world,
                (SolRay){.start = Sol_Comp_Get(world, id, ScXform)->pos, .dir = WORLD_UP, .dist = newHeight * 0.6f},
                NULL, 0.2f))
            return;
    }
    body->dims.y                              = newHeight;
    Sol_Comp_Get(world, id, ScModel)->yOffset = newHeight * -0.5f;
}

void GroundCheck(World *world, int id, ScMove3 *move, float fdt)
{
    ScXform *xform = Sol_Comp_Get(world, id, ScXform);
    ScBody3 *body  = Sol_Comp_Get(world, id, ScBody3);

    // Start from center-bottom of the body
    vec3s origin = vecAdd(xform->pos, vecSca(WORLD_DOWN, body->dims.y * 0.4f));

    move->groundNorm        = (vec3s){0, 0, 0};
    SolRayResult results[9] = {0};
    for (int j = 0; j < 9; j++)
    {
        // Rotate the local offset by the entity's rotation
        vec3s rotated_offset = glms_quat_rotatev(xform->rot, VECTOR_RADIAL_DIRECTIONS[j]);

        vec3s pos = vecAdd(origin, vecSca(rotated_offset, body->dims.x * 0.95f));

        Sol_Raycast1(world,
                     (SolRay){
                         .start     = pos,
                         .dir       = WORLD_DOWN,
                         .dist      = body->dims.y * 0.2f,
                         .ignoreEnt = id,
                         .mask      = 0,
                     },
                     &results[j]);
    }
    float flattestNorm = -1.0f;
    int   idx          = 0;
    for (int j = 0; j < 9; j++)
    {
        if (results[j].norm.y > flattestNorm)
        {
            flattestNorm = results[j].norm.y;
            idx          = j;
        }
    }

    move->groundNorm = results[idx].norm;
    move->groundDot  = flattestNorm;

    if (move->groundDot > MAX_WALK_ANGLE)
    {
        move->airtime = 0;
        move->groundtime += fdt;
    }
    else
    {
        move->groundtime = 0;
        move->airtime += fdt;
    }
}

bool Sol_Move3_SetState(World *world, int id, MoveState state)
{
    ScMove3         *move     = Sol_Comp_Get(world, id, ScMove3);
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

float Sol_Move3_GetBaseSpeed(World *world, int id)
{
    ScMove3 *move = Sol_Comp_Get(world, id, ScMove3);
    return MOVE_STATE_FORCES[move->kind][move->state].speed;
}