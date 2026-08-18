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
#include "physx/s_body2d.h"
#include "controller/s_controller.h"

#define WALKABLE_SLOPE 0.7f

static void CheckGround(World *world, int id, float fdt, CompMovement *movement)
{
    float flattestNorm  = Sol_Physx_Get_Ground_Dot(world, id);
    movement->groundDot = flattestNorm;
    if (flattestNorm > WALKABLE_SLOPE)
    {
        movement->airtime = 0;
        movement->groundtime += fdt;
    }
    else
    {
        movement->groundtime = 0;
        movement->airtime += fdt;
    }
}

static void Movement_Prestep(World *world, double dt, double time)
{
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if (world->masks[id] & BITC(HAS_MOVEMENT))
        {
            CompMovement *movement = &world->movements[id];
            movement->speedMod     = 1.0f;
            movement->gravityMod   = 1.0f;
        }
    }
}

static int  required_3d_step = BITC(HAS_MOVEMENT) | BITC(HAS_BODY3) | BITC(HAS_CONTROLLER);
static void Movement3d_Step(World *world, double dt, double time)
{
    float fdt = (float)dt;
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required_3d_step) != required_3d_step)
            continue;

        if (world->replications[id].auth == NETAUTH_REMOTE)
            continue;

        CompMovement *movement = &world->movements[id];
        movement->stateData[movement->state].elapsed += dt;

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

            MOVE_STATE_FUNCS[movement->state].update(world, id, dt);

            vec3s vel             = Sol_Physx_GetVel(world, id);
            vec3s wishdir         = Sol_GetWishdir(world, id);
            float finalSpeed      = forces->speed * movement->speedMod;
            float finalFriction   = forces->friction * movement->frictionMod;
            body->gravity.y       = forces->gravity * movement->gravityMod;
            movement->lastMoveDir = wishdir;

            switch (movement->state)
            {
            case MOVE_STUN:
                body->gravity.y *= 1.33f;
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
            default:
                if (vel.y < 0)
                    body->gravity.y *= 1.33f;
                vel = ApplyFriction3(wishdir, vel, finalFriction, fdt);
                vel = ApplyAccel3(wishdir, vel, finalSpeed, forces->accell, fdt);
                Sol_Physx_SetVellat(world, id, vel);
            }

            if (movement->state != MOVE_JUMP)
                CheckGround(world, id, fdt, movement);

            Knockback(world, id, fdt);
            RestoreFriction(world, id, movement, fdt);
        }
        CrouchHeight(world, id, fdt);

        if (id == 1)
        {
            float speed = glms_vec3_norm(Sol_Physx_GetVel(world, id));
            Sol_Debug_Add("Velocity", speed);
            Sol_Debug_Add("State", movement->state);
        }
    }
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
            CompMovement         *movement   = &world->movements[id];
            CompBody2d           *body       = &world->body2d[id];
            CompController       *controller = &world->controllers[id];
            const MoveStateForce *force      = &MOVE_STATE_FORCES[movement->kind][movement->state];

            vec2s vel     = body->vel;
            vec2s wishdir = controller->wishdir2d;

            switch (movement->state)
            {
            case MOVE_IDLE:
                if (glms_vec2_norm(wishdir) > 0)
                    movement->state = MOVE_WALK;
                break;
            }

            vel = glms_vec2_add(vel, glms_vec2_scale(wishdir, force->accell * fdt));
            Sol_Body2d_SetVel(world, id, vel);
        }
    }
}

// ############## PUBLIC ##################

void Sol_Movement_Init(World *world)
{
    world->movements = calloc(MAX_ENTS, sizeof(CompMovement));
    for (int i = 0; i < MAX_ENTS; i++)
    {
        world->movements[i].stateData->as.slide.boost = 3.0f;
    }

    WAddPrestep(world) = Movement_Prestep;
    WAddStep(world)    = Movement3d_Step;
    WAddStep(world)    = Movement2d_Step;
}

void Sol_Movement_Add(World *world, int id, MovementKind kind)
{
    if (!WHasSys(world, WORLD_SYS_MOVEMENT))
        return;
    CompMovement movement = {
        .kind = kind,
    };
    if (WHasB(world, id, HAS_BODY3))
    {
        movement.baseHeight   = Sol_Physx_GetHeight(world, id);
        movement.targetHeight = movement.baseHeight;
    }
    movement.frictionMod = 1.0f;
    movement.speedMod    = 1.0f;

    world->masks[id] |= BITC(HAS_MOVEMENT);
    world->movements[id] = movement;

    Sol_Movement_SetState(world, id, MOVE_IDLE);
}

u32 Sol_Movement_GetState(World *world, int id)
{
    return world->movements[id].state;
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

void Sol_Movement_SetSpeedMod(World *world, int id, float amnt)
{
    world->movements[id].speedMod *= amnt;
}
void Sol_Movement_SetKnockback(World *world, int id, vec3s vel, float duration)
{
    world->movements[id].knockDur = duration;
    world->movements[id].knockVel = vel;
}
float Sol_Movement_GetGroundtime(World *world, int id)
{
    return world->movements[id].groundtime;
}

float Sol_Movement_GetAirtime(World *world, int id)
{
    return world->movements[id].airtime;
}
