#include "sol_core.h"

#include "movement_i.h"

void Sol_Movement_Init(World *world)
{
    WAddPrestep(world) = Sol_Movement_Prestep;
    WAddStep(world)    = Sol_System_Movement_3d_Step;

    world->movements = calloc(MAX_ENTS, sizeof(CompMovement));
}

void Sol_Movement_Add(World *world, int id, MovementDesc desc)
{
    world->masks[id] |= HAS_MOVEMENT;
    CompMovement *movementComp = &world->movements[id];
    movementComp->configId     = desc.configId;
    Sol_Movement_SetState(world, id, MOVE_IDLE);
    movementComp->baseHeight = movementComp->targetHeight = Sol_Physx_GetHeight(world, id);
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
        if ((world->masks[id] & required) == required)
        {
            CompMovement         *movement   = &world->movements[id];
            const MoveStateForce *forces     = &MOVE_STATE_FORCES[movement->configId][movement->moveState];
            float                 finalSpeed = forces->speed * movement->speedMod;

            vec3s vel      = Sol_Physx_GetVel(world, id);
            vec3s wishdir  = Sol_GetWishdir(world, id);
            vec3s slopeDir = ProjectOntoGround(world, id, wishdir);

            switch (movement->moveState)
            {
            case MOVE_WALK:
                vel = ApplyFriction3(slopeDir, vel, forces->friction, fdt);
                vel = ApplyAccel3(slopeDir, vel, finalSpeed, forces->accell, fdt);
                Sol_Physx_SetVel(world, id, vel);
                break;
            default:
                vel = ApplyFriction3(wishdir, vel, forces->friction, fdt);
                vel = ApplyAccel3(wishdir, vel, finalSpeed, forces->accell, fdt);
                Sol_Physx_SetVellat(world, id, vel);
            }
            CrouchHeight(world, id, fdt);

            MOVE_STATE_FUNCS[movement->moveState].update(world, id, dt);
        }
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
    movement->moveState = nextState;
    nextfunc->enter(world, id);

    Sol_Physx_SetGrav(world, id, (vec3s){0, -MOVE_STATE_FORCES[movement->configId][movement->moveState].gravity, 0});

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
            const MoveStateForce *force    = &MOVE_STATE_FORCES[movement->configId][movement->moveState];

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
