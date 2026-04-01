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
            CompXform *xform = &world->xforms[id];
            CompBody *body = &world->bodies[id];
            const MoveStateFunc *funcs = &MOVE_STATE_FUNCS[movement->configId][movement->moveState];
            CompController *controller = &world->controllers[id];
            
            xform->rot = Sol_Quat_FromYawPitch(controller->yaw, -controller->pitch);

            vec3s latwishdir = controller->wishdir;
            latwishdir.y = 0;
            latwishdir = glms_vec3_normalize(latwishdir);
            movement->wishdir = latwishdir;

            funcs->update(world, id, dt);
            Sol_Debug_Add("Velocity", glms_vec3_norm(body->vel));
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
            const MoveStateForce *force = &MOVE_STATE_FORCES[movement->configId][movement->moveState];

            vec3s vel = body->vel;
            vec2s wishdir = controller->wishdir2;

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

// clang-format off
const MoveStateForce MOVE_STATE_FORCES[MOVE_CONFIG_COUNT][MOVE_STATE_COUNT] = {
    [MOVE_CONFIG_PLAYER]    = {
        [MOVE_IDLE]         = {.speed =  0,      .accell = 0,        .friction = 15.0f,  .gravity = 0        },
        [MOVE_WALK]         = {.speed =  6.0f,   .accell = 20.0f,    .friction =  5.0f,  .gravity = 9.81f    },
        [MOVE_FALL]         = {.speed =  6.0f,   .accell =  5.0f,    .friction =  0.0f,  .gravity = 9.81f    },
        [MOVE_DASH]         = {.speed = 24.0f,   .accell = 32.0f,    .friction =  0.0f,  .gravity = 0        },
        [MOVE_JUMP]         = {.speed =  6.0f,   .accell =  5.0f,    .friction =  0.0f,  .gravity = 9.81f    },
        [MOVE_SLIDE]        = {.speed =  6.0f,   .accell =  5.0f,    .friction =  1.0f,  .gravity = 9.81f    },
        [MOVE_FLY]          = {.speed =  6.0f,   .accell =  5.0f,    .friction =  1.0f,  .gravity = 0        },
    },
    [MOVE_CONFIG_WIZARD]    = {
        [MOVE_IDLE]         = {.speed = 0,      .accell = 0,        .friction = 15.0f,  .gravity = 0        },
        [MOVE_WALK]         = {.speed = 5.0f,   .accell = 20.0f,    .friction =  5.0f,  .gravity = 9.81f    },
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
            .canExit = Sol_Movement_Idle_CanExit,
            .canEnter = Sol_Movement_Idle_CanEnter,
        },
        [MOVE_WALK] = {
            .enter = Sol_Movement_Walk_Enter,
            .exit = Sol_Movement_Walk_Exit,
            .update = Sol_Movement_Walk_Update,
            .canExit = Sol_Movement_Walk_CanExit,
            .canEnter = Sol_Movement_Walk_CanEnter,
        },
        [MOVE_FALL] = {
            .enter = Sol_Movement_Fall_Enter,
            .exit = Sol_Movement_Fall_Exit,
            .update = Sol_Movement_Fall_Update,
            .canExit = Sol_Movement_Fall_CanExit,
            .canEnter = Sol_Movement_Fall_CanEnter,
        },
        [MOVE_JUMP] = {
            .enter = Sol_Movement_Jump_Enter,
            .exit = Sol_Movement_Jump_Exit,
            .update = Sol_Movement_Jump_Update,
            .canExit = Sol_Movement_Jump_CanExit,
            .canEnter = Sol_Movement_Jump_CanEnter,
        },
        [MOVE_DASH] = {
            .enter = Sol_Movement_Dash_Enter,
            .exit = Sol_Movement_Dash_Exit,
            .update = Sol_Movement_Dash_Update,
            .canExit = Sol_Movement_Dash_CanExit,
            .canEnter = Sol_Movement_Dash_CanEnter,
        },
    },
    [MOVE_CONFIG_WIZARD] = {
        [MOVE_IDLE] = {
            .enter = Sol_Movement_Idle_Enter,
            .exit = Sol_Movement_Idle_Exit,
            .update = Sol_Movement_Idle_Update,
            .canExit = Sol_Movement_Idle_CanExit,
            .canEnter = Sol_Movement_Idle_CanEnter,
        },
        [MOVE_WALK] = {
            .enter = Sol_Movement_Walk_Enter,
            .exit = Sol_Movement_Walk_Exit,
            .update = Sol_Movement_Walk_Update,
            .canExit = Sol_Movement_Walk_CanExit,
            .canEnter = Sol_Movement_Walk_CanEnter,
        },
    },
};