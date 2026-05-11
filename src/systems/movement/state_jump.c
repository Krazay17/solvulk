#include "sol_core.h"

#define JUMP_VEL 8.0f
#define JUMP_ALPHAMOD 1.66f
#define JUMP_TIMER 0.2f
#define JUMP_BUFFER 0.1f
#define JUMP_GROUND_COOLDOWN 0.1f

void Sol_Movement_Jump_Update(World *world, int id, float dt)
{
    CompMovement *movement = &world->movements[id];
    CompBody     *body     = &world->bodies[id];
    movement->stateTimer += dt;
    float alpha = JUMP_ALPHAMOD - (movement->stateTimer / JUMP_TIMER);

    if (movement->stateTimer >= JUMP_TIMER)
    {
        if (Sol_Movement_SetState(world, id, MOVE_IDLE))
            return;
    }
    // Sol_Physx_SetVelY(world, id, JUMP_VEL * alpha);
}

void Sol_Movement_Jump_Enter(World *world, int id)
{
    CompMovement *movement = &world->movements[id];
    movement->stateTimer   = 0;
    movement->hasJumped    = true;
    AnimDesc desc          = {.anim = ANIM_JUMP, .layerId = ANIM_LAYER_BASE};
    Sol_Model_PlayAnim(world, id, desc);

    Sol_Physx_SetVelY(world, id, 10.0f);
}

void Sol_Movement_Jump_Exit(World *world, int id)
{
    CompMovement *movement = &world->movements[id];
    movement->hasJumped    = false;
}

bool Sol_Movement_Jump_CanEnter(World *world, int id, int last)
{
    CompMovement *movement = &world->movements[id];
    CompBody     *body     = &world->bodies[id];
    CompAbility  *ability  = &world->abilities[id];

    return !movement->hasJumped && body->airtime < JUMP_BUFFER && !(ability->state & ABILITY_STATE_DASH);
}

bool Sol_Movement_Jump_CanExit(World *world, int id, int next)
{
    return true;
}
