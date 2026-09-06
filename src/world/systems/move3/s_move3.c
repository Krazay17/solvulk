#include "s_move3.h"
#include "world.h"
#include "sol_core.h"

#include <omp.h>

void Move3_Step(World *world, double dt)
{
    float fdt = (float)dt;
    int   i;

    SparseSet_ScMove3 *set = Sol_Comp_Set(world, ScMove3);
#pragma omp parallel for schedule(dynamic)
    for (i = 0; i < set->cnt; i++)
    {
        int      id   = set->dense[i];
        ScMove3 *move = &set->data[i];

        if (!Sol_Comp_Has(world, id, ScCmd) || !Sol_Comp_Has(world, id, ScBody3))
            continue;

        ScCmd   *cmd   = Sol_Comp_Get(world, id, ScCmd);
        ScBody3 *body3 = Sol_Comp_Get(world, id, ScBody3);

        const MoveStateForce *forces     = &MOVE_STATE_FORCES[move->kind][move->state];
        bool                  isJumpDown = cmd->actionState & BITC(ACTION_JUMP);
        vec3s                 vel        = body3->vel;
        move->vel                        = body3->vel;
        vec3s wishdir                    = cmd->wishdir;

        GroundCheck(world, id, move, fdt);

        if (isJumpDown && !move->jumpPressedLastFrame)
            move->wantsJump = true;
        else if (!isJumpDown)
            move->wantsJump = false;

        move->jumpPressedLastFrame = isJumpDown;
        move->lastMoveDir          = wishdir;

        Move3_EvaluateState(world, id, move, cmd);

        move->stateData[move->state].elapsed += dt;
        if (MOVE_STATE_FUNCS[move->state].update)
            MOVE_STATE_FUNCS[move->state].update(world, id, move, cmd, dt);

        float finalSpeed    = forces->speed; // * move->speedMod;
        float finalFriction = forces->friction * move->frictionMod;
        body3->gravity.y    = forces->gravity; // * move->gravityMod;

        switch (move->state)
        {
        case MOVE_MANTLE:
        case MOVE_LANDING:
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
                (SolRay){ .start = Sol_Comp_Get(world, id, ScXform)->pos, .dir = WORLD_UP, .dist = newHeight * 0.6f },
                NULL, 0.2f))
            return;
    }
    body->dims.y                              = newHeight;
    Sol_Comp_Get(world, id, ScModel)->yOffset = newHeight * -0.5f;
}

struct GoodRay
{
    float dist;
    vec3s norm;
};
void GroundCheck(World *world, int id, ScMove3 *move, float fdt)
{
    ScXform *xform = Sol_Comp_Get(world, id, ScXform);
    ScBody3 *body  = Sol_Comp_Get(world, id, ScBody3);

    // Start from center-bottom of the body
    vec3s origin = xform->pos; // vecAdd(xform->pos, vecSca(WORLD_DOWN, body->dims.y * 0.4f));

    move->groundNorm        = (vec3s){ 0, 0, 0 };
    SolRayResult results[9] = { 0 };
    for (int j = 0; j < 9; j++)
    {
        // Rotate the local offset by the entity's rotation
        vec3s rotated_offset = glms_quat_rotatev(xform->rot, VECTOR_RADIAL_DIRECTIONS[j]);

        vec3s pos = vecAdd(origin, vecSca(rotated_offset, body->dims.x * 0.95f));

        Sol_Raycast1(world,
                     (SolRay){
                         .start     = pos,
                         .dir       = WORLD_DOWN,
                         .dist      = 6.0f,
                         .ignoreEnt = id,
                         .mask      = 0,
                     },
                     &results[j]);
    }
    struct GoodRay best_ray = { .dist = 1e9f, .norm = (vec3s){ 0 } };
    for (int j = 0; j < 9; j++)
    {
        float dist = results[j].dist;
        vec3s norm = results[j].norm;

        if (norm.y > WALKABLE_SLOPE && dist < best_ray.dist)
        {
            best_ray.dist = dist;
            best_ray.norm = norm;
        }
    }

    move->groundNorm = best_ray.norm;
    move->groundDist = best_ray.dist;

    if (move->groundNorm.y > WALKABLE_SLOPE && (move->groundDist < (body->dims.y * 0.5f + MAX_WALK_DISTANCE_BUFFER)))
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

void Move3_EvaluateState(World *world, int id, ScMove3 *move, ScCmd *cmd)
{
    MoveState            current_state      = move->state;
    const MoveStateFunc *current_state_func = &MOVE_STATE_FUNCS[current_state];

    for (int i = 0; i < MOVE_STATE_COUNT; i++)
    {
        MoveState            target_state      = MOVE_STATE_PRIORITY[i];
        const MoveStateFunc *target_state_func = &MOVE_STATE_FUNCS[target_state];
        if (current_state_func->canExit && !current_state_func->canExit(world, id, move, cmd, target_state))
            continue;
        if (target_state_func->canEnter && !target_state_func->canEnter(world, id, move, cmd, current_state))
            continue;
        if (current_state == target_state)
            break;
        // sollog(current_state_func->canExit(world, id, move, cmd, target_state),
        //        target_state_func->canEnter(world, id, move, cmd, current_state), i);

        Move3_CommitState(world, id, target_state, current_state_func, target_state_func, move, cmd);
        break;
    }
    if (current_state != move->state)
        sollog(move->state);
}

void Move3_CommitState(World *world, int id, MoveState target_state, const MoveStateFunc *current_state_func,
                       const MoveStateFunc *target_state_func, ScMove3 *move, ScCmd *cmd)
{
    if (current_state_func->exit)
        current_state_func->exit(world, id, move, cmd);

    move->stateData[move->state].lastExited = world->tickTime;

    move->state = target_state;

    move->stateData[move->state].lastEntered = world->tickTime;
    move->stateData[move->state].elapsed     = 0.0f;

    if (target_state_func->enter)
        target_state_func->enter(world, id, move, cmd);
}

bool Sol_Move3_SetState(World *world, int id, MoveState target_state)
{
    ScMove3 *move = Sol_Comp_Get(world, id, ScMove3);
    ScCmd   *cmd  = Sol_Comp_Get(world, id, ScCmd);

    const MoveState      current_state      = move->state;
    const MoveStateFunc *current_state_func = &MOVE_STATE_FUNCS[current_state];
    const MoveStateFunc *target_state_func  = &MOVE_STATE_FUNCS[target_state];

    if (current_state_func->canExit && !current_state_func->canExit(world, id, move, cmd, target_state))
        return false;
    if (target_state_func->canEnter && !target_state_func->canEnter(world, id, move, cmd, current_state))
        return false;

    // printf("LastState: %d, CurrentState: %d\n", move->state, state);

    if (current_state_func->exit)
        current_state_func->exit(world, id, move, cmd);

    move->stateData[move->state].lastExited = world->tickTime;

    move->state = target_state;

    move->stateData[move->state].lastEntered = world->tickTime;
    move->stateData[move->state].elapsed     = 0.0f;

    if (target_state_func->enter)
        target_state_func->enter(world, id, move, cmd);

    return true;
}

float Sol_Move3_GetBaseSpeed(World *world, int id)
{
    ScMove3 *move = Sol_Comp_Get(world, id, ScMove3);
    return MOVE_STATE_FORCES[move->kind][move->state].speed;
}