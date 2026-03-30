#include <cglm/struct.h>
#include <math.h>

#include "sol_core.h"

void Sol_System_Movement_3d_Step(World *world, double dt, double time)
{
    float fdt = (float)dt;

    int required = HAS_MOVEMENT | HAS_XFORM | HAS_BODY3 | HAS_CONTROLLER;
    for (int i = 0; i < world->activeCount; ++i)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) == required)
        {
            CompMovement *movement = &world->movements[id];
            CompBody *body = &world->bodies[id];
            CompController *controller = &world->controllers[id];
            const MoveStateForce *forces = MOVE_STATE_FORCES[movement->configId];
            const MoveStateFunc *funcs = MOVE_STATE_FUNCS[movement->configId];

            vec3s vel = body->vel;
            vec3s wishdir = controller->wishdir;

            // switch (movement->moveState)
            // {
            // case MOVE_IDLE:
            //     if (glms_vec3_norm(wishdir) > 0)
            //         movement->moveState = MOVE_WALK;
            //     vel = ApplyFriction3((vec3s){0, 0, 0}, vel, forces->friction, dt);
            //     break;
            // case MOVE_WALK:
            //     if (glms_vec3_norm(wishdir) == 0)
            //         movement->moveState = MOVE_IDLE;
            //     if (controller->actionState & ACTION_JUMP)
            //         movement->moveState = MOVE_JUMP;
            //     vec3s latwishdir = wishdir;
            //     latwishdir.y = 0;
            //     latwishdir = glms_vec3_normalize(latwishdir);
            //     vel = ApplyFriction3(latwishdir, vel, forces->friction, dt);
            //     vel = ApplyAccel3(latwishdir, vel, forces->speed, forces->accell, dt);
            //     break;
            // case MOVE_JUMP:
            // }

            body->vel = vel;
        }
    }
}

void Sol_System_Movement_2d_Step(World *world, double dt, double time)
{
    float fdt = (float)dt;
    int required = HAS_MOVEMENT | HAS_XFORM | HAS_BODY2 | HAS_CONTROLLER;
    for (int i = 0; i < world->activeCount; ++i)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) == required)
        {
            CompMovement *movement = &world->movements[id];
            CompBody *body = &world->bodies[id];
            CompController *controller = &world->controllers[id];
            // const MoveStateForces *forces = &MOVE_CONFIGS[movement->configId].stateForces[movement->moveState];

            vec3s vel = body->vel;
            vec2s wishdir = controller->wishdir2;

            switch (movement->moveState)
            {
            case MOVE_IDLE:
                if (glms_vec2_norm(wishdir) > 0)
                    movement->moveState = MOVE_WALK;
                break;
            }

            // vel = glms_vec3_add(body->vel, glms_vec3_scale((vec3s){wishdir.x, wishdir.y, 0}, forces->accell * fdt));

            body->vel = vel;
        }
    }
}

// clang-format off
const MoveStateForce MOVE_STATE_FORCES[MOVE_CONFIG_COUNT][MOVE_STATE_COUNT] = {
    [MOVE_CONFIG_PLAYER]    = {
        [MOVE_IDLE]         = {.speed = 0,      .accell = 0,        .friction = 15.0f,  .gravity = 0        },
        [MOVE_WALK]         = {.speed = 5.0f,   .accell = 20.0f,    .friction = 5.0f,   .gravity = 9.81f    },
        [MOVE_FALL]         = {.speed = 5.0f,   .accell =  5.0f,    .friction =  0.0f,  .gravity = 9.81f    },
        [MOVE_JUMP]         = {.speed = 5.0f,   .accell =  5.0f,    .friction =  0.0f,  .gravity = 9.81f    },
        [MOVE_SLIDE]        = {.speed = 5.0f,   .accell =  5.0f,    .friction =  1.0f,  .gravity = 9.81f    },
        [MOVE_FLY]          = {.speed = 5.0f,   .accell =  5.0f,    .friction =  1.0f,  .gravity = 0        },
                            },
    [MOVE_CONFIG_WIZARD]    = {
        [MOVE_IDLE]         = {.speed = 0,      .accell = 0,        .friction = 15.0f,  .gravity = 0        },
        [MOVE_WALK]         = {.speed = 5.0f,   .accell = 20.0f,    .friction = 5.0f,   .gravity = 9.81f    },
        [MOVE_FALL]         = {.speed = 5.0f,   .accell =  5.0f,    .friction =  0.0f,  .gravity = 9.81f    },
        [MOVE_JUMP]         = {.speed = 5.0f,   .accell =  5.0f,    .friction =  0.0f,  .gravity = 9.81f    },
        [MOVE_SLIDE]        = {.speed = 5.0f,   .accell =  5.0f,    .friction =  1.0f,  .gravity = 9.81f    },
        [MOVE_FLY]          = {.speed = 5.0f,   .accell =  5.0f,    .friction =  1.0f,  .gravity = 0        },
                            },
};
// clang-format on

const MoveStateFunc MOVE_STATE_FUNCS[MOVE_CONFIG_COUNT][MOVE_STATE_COUNT] = {
    [MOVE_CONFIG_PLAYER] = {
        [MOVE_IDLE] = {
            .enter = Sol_Movement_Idle_Enter,
            .exit = Sol_Movement_Idle_Exit,
            .update = Sol_Movement_Idle_Update,
        },
        [MOVE_WALK] = {
            .enter = Sol_Movement_Walk_Enter,
            .exit = Sol_Movement_Walk_Exit,
            .update = Sol_Movement_Walk_Update,
        },
    },
    [MOVE_CONFIG_WIZARD] = {
        [MOVE_IDLE] = {
            .enter = Sol_Movement_Idle_Enter,
            .exit = Sol_Movement_Idle_Exit,
            .update = Sol_Movement_Idle_Update,
        },
    },
};