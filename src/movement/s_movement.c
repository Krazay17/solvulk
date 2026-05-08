#include "sol_core.h"

const MoveStateForce MOVE_STATE_FORCES[MOVE_CONFIG_COUNT][MOVE_STATE_COUNT] = {
    [MOVE_CONFIG_PLAYER] =
        {
            [MOVE_IDLE]  = {.speed = 0, .accell = 0, .friction = 20.0f, .gravity = 0},
            [MOVE_WALK]  = {.speed = 6.0f, .accell = 20.0f, .friction = 10.0f, .gravity = 9.81f},
            [MOVE_FALL]  = {.speed = 6.0f, .accell = 4.0f, .friction = 0.0f, .gravity = 9.81f},
            [MOVE_JUMP]  = {.speed = 6.0f, .accell = 10.0f, .friction = 0.0f, .gravity = 9.81f},
            [MOVE_SLIDE] = {.speed = 6.0f, .accell = 5.0f, .friction = 1.0f, .gravity = 9.81f},
            [MOVE_FLY]   = {.speed = 6.0f, .accell = 10.0f, .friction = 1.0f, .gravity = 0},
        },
    [MOVE_CONFIG_WIZARD] =
        {
            [MOVE_IDLE]  = {.speed = 0, .accell = 0, .friction = 15.0f, .gravity = 0},
            [MOVE_WALK]  = {.speed = 4.0f, .accell = 20.0f, .friction = 10.0f, .gravity = 9.81f},
            [MOVE_FALL]  = {.speed = 4.0f, .accell = 5.0f, .friction = 0.0f, .gravity = 9.81f},
            [MOVE_JUMP]  = {.speed = 4.0f, .accell = 5.0f, .friction = 0.0f, .gravity = 9.81f},
            [MOVE_SLIDE] = {.speed = 4.0f, .accell = 5.0f, .friction = 1.0f, .gravity = 9.81f},
            [MOVE_FLY]   = {.speed = 4.0f, .accell = 5.0f, .friction = 1.0f, .gravity = 0},
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
    world->movements = calloc(MAX_ENTS, sizeof(CompMovement));
}

void Sol_Movement_Add(World *world, int id, MovementDesc desc)
{
    world->masks[id] |= HAS_MOVEMENT;
    CompMovement *movementComp = &world->movements[id];
    movementComp->configId     = desc.configId;
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
            CompMovement *movement = &world->movements[id];
            CompXform    *xform    = &world->xforms[id];

            xform->quat = Sol_Quat_FromYawPitch(Sol_GetYaw(world, id), 0); // -controller->pitch

            const MoveStateForce *forces = &MOVE_STATE_FORCES[movement->configId][movement->moveState];
            if (forces)
            {
                vec3s vel        = Sol_Physx_GetVel(world, id);
                vec3s wishdir    = Sol_GetWishdir(world, id);
                vec3s latwishdir = wishdir;
                latwishdir.y     = 0;
                latwishdir       = glms_vec3_normalize(latwishdir);
                float dot        = glms_vec3_dot(latwishdir, Sol_Physx_GetGround(world, id));

                vec3s slopeDir = glms_vec3_sub(latwishdir, glms_vec3_scale(Sol_Physx_GetGround(world, id), dot));
                vel            = ApplyFriction3(slopeDir, vel, forces->friction, fdt);
                vel            = ApplyAccel3(slopeDir, vel, forces->speed, forces->accell, fdt);
                Sol_Physx_SetVel(world, id, vel);
            }

            MOVE_STATE_FUNCS[movement->moveState].update(world, id, dt);
        }
    }
    if (world->playerID > 0)
    {
        float speed = glms_vec3_norm(world->bodies[world->playerID].vel);
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
            CompBody             *body     = &world->bodies[id];
            const MoveStateForce *force    = &MOVE_STATE_FORCES[movement->configId][movement->moveState];

            vec3s vel     = body->vel;
            vec2s wishdir = Sol_GetWishdir2(world, id);

            switch (movement->moveState)
            {
            case MOVE_IDLE:
                if (glms_vec2_norm(wishdir) > 0)
                    movement->moveState = MOVE_WALK;
                break;
            }

            vel = glms_vec3_add(body->vel, glms_vec3_scale((vec3s){wishdir.x, wishdir.y, 0}, force->accell * fdt));

            body->vel = vel;
        }
    }
}
