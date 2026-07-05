/*
 * File: s_movement.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-06-28
 * Example of how systems operate with forward declared flat component pointers
 */

#include "movement_i.h"
#include "sol_core.h"
#include "sol_math.h"
#include "replication/s_replication.h"
#include "world.h"
#include "physx/s_body.h"
#include "controller/s_controller.h"

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

static void Movement_Prestep(World *world, double dt, double time);
static void Movement3d_Step(World *world, double dt, double time);
static void Movement2d_Step(World *world, double dt, double time);

void Sol_Movement_Init(World *world)
{
    world->movements = calloc(MAX_ENTS, sizeof(CompMovement));

    WAddPrestep(world) = Movement_Prestep;
    WAddStep(world)    = Movement3d_Step;
}

void Sol_Movement_Add(World *world, int id, MovementKind kind)
{
    CompMovement movement = {
        .kind = kind,
    };
    movement.baseHeight   = Sol_Physx_GetHeight(world, id);
    movement.targetHeight = movement.baseHeight;
    movement.frictionMod  = 1.0f;
    movement.speedMod     = 1.0f;

    world->masks[id] |= BITC(HAS_MOVEMENT);
    world->movements[id] = movement;

    Sol_Movement_SetState(world, id, MOVE_IDLE);
}

u32 Sol_Movement_GetState(World *world, int id)
{
    return world->movements[id].state;
}

static void Movement_Prestep(World *world, double dt, double time)
{
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if (world->masks[id] & BITC(HAS_MOVEMENT))
            world->movements[id].speedMod = 1.0f;
    }
}

static void Movement3d_Step(World *world, double dt, double time)
{
    float fdt      = (float)dt;
    int   required = BITC(HAS_MOVEMENT);
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;

        if (world->replications[id].auth == NETAUTH_REMOTE)
            continue;

        CompMovement *movement = &world->movements[id];
        if (world->masks[id] & BITC(HAS_BODY3))
        {
            CompBody             *body   = &world->bodies[id];
            const MoveStateForce *forces = &MOVE_STATE_FORCES[movement->kind][movement->state];

            bool isJumpDown = Sol_Controller_IsActionState(world, id, ACTION_JUMP);

            if (isJumpDown && !movement->jumpPressedLastFrame)
                movement->wantsJump = true;
            else if (!isJumpDown)
                movement->wantsJump = false;
            movement->jumpPressedLastFrame = isJumpDown;

            vec3s vel           = Sol_Physx_GetVel(world, id);
            vec3s wishdir       = Sol_GetWishdir(world, id);
            float finalSpeed    = forces->speed * movement->speedMod;
            float finalFriction = forces->friction * movement->frictionMod;
            body->gravity.y     = forces->gravity;

            switch (movement->state)
            {
            case MOVE_STUN:
                body->gravity.y *= 1.33f;
                vel = ApplyFriction3(wishdir, vel, finalFriction, fdt);
                Sol_Physx_SetVel(world, id, vel);
                break;
            case MOVE_WALK:
                vec3s slopeDir = ProjectOntoGround(world, id, wishdir);
                vel            = ApplyFriction3(slopeDir, vel, finalFriction, fdt);
                vel            = ApplyAccel3(slopeDir, vel, finalSpeed, forces->accell, fdt);
                Sol_Physx_SetVel(world, id, vel);
                break;
            default:
                if (vel.y < 0)
                    body->gravity.y *= 1.33f;
                vel = ApplyFriction3(wishdir, vel, finalFriction, fdt);
                vel = ApplyAccel3(wishdir, vel, finalSpeed, forces->accell, fdt);
                Sol_Physx_SetVellat(world, id, vel);
            }
        }

        MOVE_STATE_FUNCS[movement->state].update(world, id, dt);

        CrouchHeight(world, id, fdt);
        Knockback(world, id, fdt);
        RestoreFriction(world, id, movement, fdt);

        if (id == 1)
        {
            float speed = glms_vec3_norm(Sol_Physx_GetVel(world, id));
            Sol_Debug_Add("Velocity", speed);
            Sol_Debug_Add("State", movement->state);
        }
    }
}

void Sol_Movement_ForceState(World *world, int id, MoveState nextState)
{
    CompMovement    *movement = &world->movements[id];
    const StateFunc *prevfunc = &MOVE_STATE_FUNCS[movement->state];
    const StateFunc *nextfunc = &MOVE_STATE_FUNCS[nextState];
    prevfunc->exit(world, id);
    movement->stateData[movement->state].lastExited  = solState.gameTime;
    movement->state                                  = nextState;
    movement->stateData[movement->state].lastEntered = solState.gameTime;
    movement->stateData[movement->state].elapsed     = 0.0f;
    nextfunc->enter(world, id);
    Sol_Physx_SetGrav(world, id, (vec3s){0, -MOVE_STATE_FORCES[movement->kind][movement->state].gravity, 0});
}

bool Sol_Movement_SetState(World *world, int id, MoveState nextState)
{
    CompMovement    *movement = &world->movements[id];
    const StateFunc *prevfunc = &MOVE_STATE_FUNCS[movement->state];
    const StateFunc *nextfunc = &MOVE_STATE_FUNCS[nextState];

    if (movement->state == nextState)
        return false;
    if (!prevfunc->canExit(world, id, nextState))
        return false;
    if (!nextfunc->canEnter(world, id, movement->state, (u32)nextState, 0))
        return false;

    // printf("LastState: %d, CurrentState: %d\n", movement->state, nextState);

    prevfunc->exit(world, id);
    movement->stateData[movement->state].lastExited = solState.gameTime;

    movement->state                                  = nextState;
    movement->stateData[movement->state].lastEntered = solState.gameTime;
    movement->stateData[movement->state].elapsed     = 0.0f;
    nextfunc->enter(world, id);

    Sol_Physx_SetGrav(world, id, (vec3s){0, -MOVE_STATE_FORCES[movement->kind][movement->state].gravity, 0});

    return true;
}

static int  movement2d_step_required = BITC(HAS_MOVEMENT) | BITC(HAS_BODY2) | BITC(HAS_CONTROLLER);
static void Movement2d_Step(World *world, double dt, double time)
{
    float fdt = (float)dt;
    for (int i = 0; i < world->activeCount; ++i)
    {
        int id = world->activeEntities[i];
        if (WHas(world, id, movement2d_step_required))
        {
            return;
            CompMovement         *movement = &world->movements[id];
            const MoveStateForce *force    = &MOVE_STATE_FORCES[movement->kind][movement->state];

            vec3s vel     = Sol_Physx_GetVel(world, id);
            vec3s wishdir = Sol_GetWishdir2(world, id);

            switch (movement->state)
            {
            case MOVE_IDLE:
                if (glms_vec3_norm(wishdir) > 0)
                    movement->state = MOVE_WALK;
                break;
            }
            vel = glms_vec3_add(vel, glms_vec3_scale((vec3s){wishdir.x, wishdir.y, 0}, force->accell * fdt));
            Sol_Physx_SetVel(world, id, vel);
        }
    }
}

void Sol_Movement_SetSpeedMod(World *world, int id, float amnt)
{
    world->movements[id].speedMod *= amnt;
}
void Sol_Movement_SetKnockback(World *world, int id, vec3s vel, float duration)
{
    world->movements[id].knockDur = duration;
    world->movements[id].knockVel = vel;
}