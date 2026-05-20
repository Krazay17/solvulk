#include "sol_core.h"

#include "movement_i.h"

const MoveStateForce MOVE_STATE_FORCES[MOVE_CONFIG_COUNT][MOVE_STATE_COUNT] = {
    [MOVE_CONFIG_PLAYER] =
        {
            [MOVE_IDLE]  = {.speed = 0, .accell = 0, .friction = 20.0f, .gravity = 9.81f},
            [MOVE_WALK]  = {.speed = 6.0f, .accell = 25.0f, .friction = 8.0f, .gravity = 9.81f},
            [MOVE_FALL]  = {.speed = 4.0f, .accell = 7.0f, .friction = 0.1f, .gravity = 16.0f},
            [MOVE_JUMP]  = {.speed = 4.0f, .accell = 7.0f, .friction = 0.1f, .gravity = 9.81f},
            [MOVE_SLIDE] = {.speed = 4.0f, .accell = 7.0f, .friction = 1.0f, .gravity = 9.81f},
            [MOVE_FLY]   = {.speed = 4.0f, .accell = 7.0f, .friction = 1.0f, .gravity = 0},
        },
    [MOVE_CONFIG_WIZARD] =
        {
            [MOVE_IDLE]  = {.speed = 0, .accell = 0, .friction = 15.0f, .gravity = 0},
            [MOVE_WALK]  = {.speed = 5.0f, .accell = 20.0f, .friction = 8.0f, .gravity = 9.81f},
            [MOVE_FALL]  = {.speed = 5.0f, .accell = 3.5f, .friction = 0.1f, .gravity = 9.81f},
            [MOVE_JUMP]  = {.speed = 5.0f, .accell = 3.5f, .friction = 0.1f, .gravity = 9.81f},
            [MOVE_SLIDE] = {.speed = 5.0f, .accell = 3.5f, .friction = 1.0f, .gravity = 9.81f},
            [MOVE_FLY]   = {.speed = 5.0f, .accell = 3.5f, .friction = 1.0f, .gravity = 0},
        },
};

const StateFunc MOVE_STATE_FUNCS[MOVE_STATE_COUNT] = {
    [MOVE_IDLE] =
        {
            .update   = Sol_Movement_Idle_Update,
            .enter    = Sol_Movement_Idle_Enter,
            .exit     = Sol_Movement_Idle_Exit,
            .canExit  = Sol_Movement_Idle_CanExit,
            .canEnter = Sol_Movement_Idle_CanEnter,
        },
    [MOVE_WALK] =
        {
            .update   = Sol_Movement_Walk_Update,
            .enter    = Sol_Movement_Walk_Enter,
            .exit     = Sol_Movement_Walk_Exit,
            .canExit  = Sol_Movement_Walk_CanExit,
            .canEnter = Sol_Movement_Walk_CanEnter,
        },
    [MOVE_FALL] =
        {
            .update   = Sol_Movement_Fall_Update,
            .enter    = Sol_Movement_Fall_Enter,
            .exit     = Sol_Movement_Fall_Exit,
            .canExit  = Sol_Movement_Fall_CanExit,
            .canEnter = Sol_Movement_Fall_CanEnter,
        },
    [MOVE_JUMP] =
        {
            .update   = Sol_Movement_Jump_Update,
            .enter    = Sol_Movement_Jump_Enter,
            .exit     = Sol_Movement_Jump_Exit,
            .canExit  = Sol_Movement_Jump_CanExit,
            .canEnter = Sol_Movement_Jump_CanEnter,
        },
    [MOVE_FLY] =
        {
            .update   = Sol_Movement_Fly_Update,
            .enter    = Sol_Movement_Fly_Enter,
            .exit     = Sol_Movement_Fly_Exit,
            .canExit  = Sol_Movement_Fly_CanExit,
            .canEnter = Sol_Movement_Fly_CanEnter,
        },
};

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

            vec3s vel     = Sol_Physx_GetVel(world, id);
            vec3s wishdir = Sol_GetWishdir(world, id);

            switch (movement->moveState)
            {
            case MOVE_WALK:
                float dot      = glms_vec3_dot(wishdir, Sol_Physx_GetGround(world, id));
                vec3s slopeDir = glms_vec3_sub(wishdir, glms_vec3_scale(Sol_Physx_GetGround(world, id), dot));
                vel            = ApplyFriction3(slopeDir, vel, forces->friction, fdt);
                vel            = ApplyAccel3(slopeDir, vel, finalSpeed, forces->accell, fdt);
                Sol_Physx_SetVel(world, id, vel);
                break;
            default:
                vel = ApplyFriction3(wishdir, vel, forces->friction, fdt);
                vel = ApplyAccel3(wishdir, vel, finalSpeed, forces->accell, fdt);
                Sol_Physx_SetVellat(world, id, vel);
            }

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
    if (!nextfunc->canEnter(world, id, movement->moveState))
        return false;

    // printf("LastState: %d, CurrentState: %d\n", movement->moveState, nextState);

    prevfunc->exit(world, id);
    movement->moveState = nextState;
    nextfunc->enter(world, id);

    Sol_Physx_SetGrav(world, id, (vec3s){0, -MOVE_STATE_FORCES[movement->configId][movement->moveState].gravity, 0});

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