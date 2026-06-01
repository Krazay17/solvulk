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
    int   required = HAS_MOVEMENT | HAS_XFORM | HAS_BODY3;
    for (int i = 0; i < world->activeCount; ++i)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;

        if (world->replications[id].auth == NETAUTH_REMOTE)
            continue;

        CompMovement         *movement = &world->movements[id];
        CompBody             *body     = &world->bodies[id];
        const MoveStateForce *forces   = &MOVE_STATE_FORCES[movement->kind][movement->moveState];

        bool isJumpDown = Sol_Controller_IsActionState(world, id, ACTION_JUMP);

        if (isJumpDown && !movement->jumpPressedLastFrame)
            movement->wantsJump = true;
        else if (!isJumpDown)
            movement->wantsJump = false;
        movement->jumpPressedLastFrame = isJumpDown;

        vec3s vel        = Sol_Physx_GetVel(world, id);
        vec3s wishdir    = Sol_GetWishdir(world, id);
        float finalSpeed = forces->speed * movement->speedMod;
        body->gravity.y  = forces->gravity;

        switch (movement->moveState)
        {
        case MOVE_WALK:
            vec3s slopeDir = ProjectOntoGround(world, id, wishdir);
            vel            = ApplyFriction3(slopeDir, vel, forces->friction, fdt);
            vel            = ApplyAccel3(slopeDir, vel, finalSpeed, forces->accell, fdt);
            Sol_Physx_SetVel(world, id, vel);
            break;
        default:
            if (vel.y < 0)
                body->gravity.y *= 1.5f;
            vel = ApplyFriction3(wishdir, vel, forces->friction, fdt);
            vel = ApplyAccel3(wishdir, vel, finalSpeed, forces->accell, fdt);
            Sol_Physx_SetVellat(world, id, vel);
        }

        MOVE_STATE_FUNCS[movement->moveState].update(world, id, dt);

        CrouchHeight(world, id, fdt);
    }
    if (world->playerID > 0)
    {
        float speed = glms_vec3_norm(Sol_Physx_GetVel(world, world->playerID));
        Sol_Debug_Add("Velocity", speed);
    }
}

bool Sol_Movement_SetState(World *world, int id, MoveState nextState)
{
    CompMovement    *movement = &world->movements[id];
    const StateFunc *prevfunc = &MOVE_STATE_FUNCS[movement->moveState];
    const StateFunc *nextfunc = &MOVE_STATE_FUNCS[nextState];

    if (movement->moveState == nextState)
        return false;
    if (!prevfunc->canExit(world, id, nextState))
        return false;
    if (!nextfunc->canEnter(world, id, movement->moveState, (u32)nextState))
        return false;

    // printf("LastState: %d, CurrentState: %d\n", movement->moveState, nextState);

    prevfunc->exit(world, id);
    movement->stateData[movement->moveState].lastExited = Sol_GetGameTime();

    movement->moveState                                  = nextState;
    movement->stateData[movement->moveState].lastEntered = Sol_GetGameTime();
    movement->stateData[movement->moveState].elapsed     = 0.0f;
    nextfunc->enter(world, id);

    Sol_Physx_SetGrav(world, id, (vec3s){0, -MOVE_STATE_FORCES[movement->kind][movement->moveState].gravity, 0});

    if (id == world->playerID)
        Sol_Debug_Add("Move State", movement->moveState);

    return true;
}

void Sol_System_Movement_2d_Step(World *world, double dt, double time)
{
    float fdt      = (float)dt;
    int   required = HAS_MOVEMENT | HAS_XFORM | HAS_BODY2 | HAS_CONTROLLER;
    for (int i = 0; i < world->activeCount; ++i)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) == required)
        {
            return;
            CompMovement         *movement = &world->movements[id];
            const MoveStateForce *force    = &MOVE_STATE_FORCES[movement->kind][movement->moveState];

            vec3s vel     = Sol_Physx_GetVel(world, id);
            vec3s wishdir = Sol_GetWishdir2(world, id);

            switch (movement->moveState)
            {
            case MOVE_IDLE:
                if (glms_vec3_norm(wishdir) > 0)
                    movement->moveState = MOVE_WALK;
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
