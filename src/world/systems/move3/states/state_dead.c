#include "move3/s_move3.h"
#include "world.h"
#include "sol_math.h"

#define REMOVE_PHYSX_TIMER 20.0f
#define DESTROY_TIMER 125.0f

void Dead_State_Update(World *world, int id, float dt)
{
    SolMove3   *move = Sol_Comp_Get(world, id, SolMove3);
    MoveStateData *data = &move->stateData[move->state];

    if (data->elapsed > DESTROY_TIMER)
    {
        Sol_Destroy_Ent(world, id);
        return;
    }
    if (data->elapsed > REMOVE_PHYSX_TIMER)
    {
        world->masks[id] &= ~BITC(HAS_SolBody3);
        world->masks[id] &= ~BITC(HAS_SolBody2);
    }
}

void Dead_State_Enter(World *world, int id)
{
    SolMove3 *move  = Sol_Comp_Get(world, id, SolMove3);
    move->targetHeight = move->baseHeight * 0.6f;
}
void Dead_State_Exit(World *world, int id)
{
    SolMove3 *move  = Sol_Comp_Get(world, id, SolMove3);
    move->targetHeight = move->baseHeight;
}
bool Dead_State_CanExit(World *world, int id, u32 next)
{
    return next != MOVE_DEAD;
}
bool Dead_State_CanEnter(World *world, int id, u32 last, u32 next, int slot)
{
    return true;
}
