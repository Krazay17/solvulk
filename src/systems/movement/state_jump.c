#include "movement_system.h"
#include "sol_core.h"

#define JUMP_VEL 8.0f
#define JUMP_ALPHAMOD 1.66f
#define JUMP_TIMER 0.2f
#define JUMP_GROUND_COOLDOWN 0.1f

void Sol_Movement_Jump_Update(World *world, int id, float dt)
{
    CompMovement *movement = &world->movements[id];
    CompBody *body = &world->bodies[id];
    CompController *controller = &world->controllers[id];
    movement->stateTimer += dt;
    float alpha = JUMP_ALPHAMOD - (movement->stateTimer / JUMP_TIMER);

    if (movement->stateTimer >= JUMP_TIMER)
    {
        if (Sol_Movement_SetState(world, id, MOVE_IDLE))
            return;
    }
    body->vel.y = JUMP_VEL * alpha;
}

void Sol_Movement_Jump_Enter(World *world, int id)
{
    Sol_Model_PlayAnim(world, id, ANIM_JUMP, 0);
    CompMovement *movement = &world->movements[id];
    movement->stateTimer = 0;
}

void Sol_Movement_Jump_Exit(World *world, int id)
{
}

bool Sol_Movement_Jump_CanEnter(World *world, int id)
{
    CompBody *body = &world->bodies[id];

    return body->grounded > JUMP_GROUND_COOLDOWN;
}

bool Sol_Movement_Jump_CanExit(World *world, int id)
{
    return true;
}
