#include "sol_core.h"

#include "movement_i.h"

#define JUMP_BUFFER 0.1f

#define JUMP_VEL 9.0f
#define JUMP_DURATION 0.2f
#define DAMPING 5.0f

void Sol_Movement_Jump_Update(World *world, int id, float dt)
{
    CompMovement  *movement = &world->movements[id];
    MoveStateData *data     = &movement->stateData[MOVE_JUMP];
    data->elapsed += dt;
    float alpha = 1.0f - (data->elapsed / JUMP_DURATION);

    if (data->elapsed >= JUMP_DURATION)
    {
        Sol_Movement_SetState(world, id, MOVE_IDLE);
        return;
    }

    vec3s vel = Sol_Physx_GetVel(world, id);
    vel       = Sol_Math_InterpDir(vel, WORLD_UP, alpha, DAMPING, dt);
    Sol_Physx_SetVel(world, id, vel);
}

void Sol_Movement_Jump_Enter(World *world, int id)
{
    CompMovement *movement = &world->movements[id];
    movement->wantsJump    = false;
    AnimDesc desc          = {.anim = ANIM_JUMP, .layerId = ANIM_LAYER_BASE};
    Sol_Model_PlayAnim(world, id, desc);

    if (Sol_Physx_GetVel(world, id).y < 0)
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
