#include "sol_core.h"

#include "movement_i.h"
#include "physx/physx_i.h"

#define JUMP_VEL 12.0f
#define JUMP_ALPHAMOD 1.66f
#define JUMP_TIMER 0.2f
#define JUMP_BUFFER 0.1f
#define JUMP_GROUND_COOLDOWN 0.1f

void Sol_Movement_Jump_Update(World *world, int id, float dt)
{
    CompMovement *movement = &world->movements[id];
    movement->stateTimer += dt;
    float          alpha = JUMP_ALPHAMOD - (movement->stateTimer / JUMP_TIMER);
    MoveStateData *data  = &movement->stateData[movement->moveState];
    data->elapsed += dt;

    if (movement->stateTimer >= JUMP_TIMER)
    {
        if (Sol_Movement_SetState(world, id, MOVE_IDLE))
            return;
    }
    vec3s vel = Sol_Physx_GetVel(world, id);
    vel = Sol_Math_InterpDir(vel, WORLD_UP, data->elapsed / JUMP_TIMER, 9.5f, dt);
    
        //vel = glms_vec3_sub(vel, glms_vec3_scale(WORLD_UP, 1.2f * dt));
    Sol_Physx_SetVel(world, id, vel);
}

void Sol_Movement_Jump_Enter(World *world, int id)
{
    CompMovement *movement = &world->movements[id];
    movement->stateTimer   = 0;
    movement->wantsJump    = false;
    AnimDesc desc          = {.anim = ANIM_JUMP, .layerId = ANIM_LAYER_BASE};
    Sol_Model_PlayAnim(world, id, desc);

    Sol_Physx_SetVelY(world, id, 0);
    vec3s dir = glms_vec3_normalize(glms_vec3_lerp(Sol_Physx_GetGround(world, id), WORLD_UP, 0.9f));
    Sol_Physx_AddVel(world, id, vecSca(dir, JUMP_VEL));
}

void Sol_Movement_Jump_Exit(World *world, int id)
{
}

bool Sol_Movement_Jump_CanExit(World *world, int id, u32 next)
{
    return next != MOVE_JUMP;
}

bool Sol_Movement_Jump_CanEnter(World *world, int id, u32 last, u32 next)
{
    CompMovement *movement = &world->movements[id];
    CompBody     *body     = &world->bodies[id];
    if (!movement->wantsJump)
        return false;

    return body->airtime < JUMP_BUFFER && (Sol_Ability_GetState(world, id) != ABILITY_STATE_DASH);
}
