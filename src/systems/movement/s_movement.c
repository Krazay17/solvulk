#include "sol_core.h"

#include "movement_i.h"

void Sol_Movement_Init(World *world)
{
    WAddPrestep(world) = Sol_Movement_Prestep;
    WAddStep(world)    = Sol_System_Movement_3d_Step;

    world->movements = calloc(MAX_ENTS, sizeof(CompMovement));
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

    world->masks[id] |= HAS_MOVEMENT;
    world->movements[id] = movement;

    Sol_Movement_SetState(world, id, MOVE_IDLE);
}

void Sol_Movement_Prestep(World *world, double dt, double time)
{
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if (world->masks[id] & HAS_MOVEMENT)
            world->movements[id].speedMod = 1.0f;
    }
}

void Sol_System_Movement_3d_Step(World *world, double dt, double time)
{
    float fdt      = (float)dt;
    int   required = HAS_MOVEMENT | HAS_BODY3;
    for (int i = 0; i < world->activeCount; ++i)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;

        if (world->replications[id].auth == NETAUTH_REMOTE)
            continue;

        CompMovement         *movement = &world->movements[id];
        CompBody             *body     = &world->bodies[id];
        const MoveStateForce *forces   = &MOVE_STATE_FORCES[movement->kind][movement->state];

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

        MOVE_STATE_FUNCS[movement->state].update(world, id, dt);

        CrouchHeight(world, id, fdt);
        Knockback(world, id, fdt);
        RestoreFriction(world, id, movement, fdt);
    }
    if (world->playerID > 0)
    {
        float speed = glms_vec3_norm(Sol_Physx_GetVel(world, world->playerID));
        Sol_Debug_Add("Velocity", speed);
    }
}

void Sol_Movement_ForceState(World *world, int id, MoveState nextState)
{
    CompMovement    *movement = &world->movements[id];
    const StateFunc *prevfunc = &MOVE_STATE_FUNCS[movement->state];
    const StateFunc *nextfunc = &MOVE_STATE_FUNCS[nextState];
    prevfunc->exit(world, id);
    movement->stateData[movement->state].lastExited  = Sol_GetGameTime();
    movement->state                                  = nextState;
    movement->stateData[movement->state].lastEntered = Sol_GetGameTime();
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
    movement->stateData[movement->state].lastExited = Sol_GetGameTime();

    movement->state                                  = nextState;
    movement->stateData[movement->state].lastEntered = Sol_GetGameTime();
    movement->stateData[movement->state].elapsed     = 0.0f;
    nextfunc->enter(world, id);

    Sol_Physx_SetGrav(world, id, (vec3s){0, -MOVE_STATE_FORCES[movement->kind][movement->state].gravity, 0});

    // if (id == world->playerID)
    //     Sol_Debug_Add("Move State", movement->state);

    return true;
}

void Sol_System_Movement_2d_Step(World *world, double dt, double time)
{
    float fdt      = (float)dt;
    int   required = HAS_MOVEMENT | HAS_BODY2 | HAS_CONTROLLER;
    for (int i = 0; i < world->activeCount; ++i)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) == required)
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