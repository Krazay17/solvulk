#include "movement_i.h"
#include "world.h"
#include "sol_math.h"
#include "xform/s_xform.h"
#include "model/s_model.h"
#include "physx/s_body.h"
#include "controller/s_controller.h"
#include "ability/s_ability.h"

#define JUMP_BUFFER 0.1f

#define JUMP_VEL 10.0f
#define JUMP_DURATION 0.4f
#define DAMPING 4.5f

void Sol_Movement_Jump_Update(World *world, int id, float dt)
{
    CompMovement  *movement = &world->movements[id];
    MoveStateData *data     = &movement->stateData[MOVE_JUMP];

    if (data->elapsed >= JUMP_DURATION)
    {
        Sol_Movement_SetState(world, id, MOVE_IDLE);
        return;
    }
    else if (Sol_Controller_IsActionState(world, id, ACTION_JUMP) && data->elapsed > JUMP_DURATION * 0.1f)
        if (Sol_Movement_SetState(world, id, MOVE_WALLRUN))
            return;

    float alpha = 1.0f - (data->elapsed / JUMP_DURATION);

    vec3s vel = Sol_Physx_GetVel(world, id);
    vel       = Sol_Math_DampDir(vel, WORLD_UP, alpha, DAMPING, dt);
    Sol_Physx_SetVel(world, id, vel);
}

void Sol_Movement_Jump_Enter(World *world, int id)
{
    CompMovement *movement = &world->movements[id];
    movement->wantsJump    = false;
    movement->groundtime   = 0;
    movement->airtime      = JUMP_BUFFER;

    if (Sol_Physx_GetVel(world, id).y < 0)
        Sol_Physx_SetVelY(world, id, 0);
    vec3s dir = glms_vec3_normalize(glms_vec3_lerp(Sol_Physx_GetGround(world, id), WORLD_UP, 0.9f));
    Sol_Physx_AddVel(world, id, vecSca(dir, JUMP_VEL));

    AnimDesc desc = {.anim = ANIM_JUMP, .layerId = ANIM_LAYER_BASE};

    Sol_Model_PlayAnim(world, id, desc);
}

void Sol_Movement_Jump_Exit(World *world, int id)
{
}

bool Sol_Movement_Jump_CanExit(World *world, int id, u32 next)
{
    return next != MOVE_JUMP;
}

bool Sol_Movement_Jump_CanEnter(World *world, int id, u32 last, u32 next, int slot)
{
    CompMovement *movement = &world->movements[id];
    if (!movement->wantsJump)
        return false;

    return movement->airtime < JUMP_BUFFER && (Sol_Ability_GetState(world, id) != ABILITY_STATE_DASH);
}
