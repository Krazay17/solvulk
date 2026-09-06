#include "move3/s_move3.h"
#include "world.h"
#include "sol_math.h"

#define REMOVE_PHYSX_TIMER 20.0f
#define DESTROY_TIMER 125.0f

void Move_Dead_Update(World *world, int id, ScMove3 *move, ScCmd *cmd, float dt)
{
    MoveStateData *data = &move->stateData[move->state];

    if (data->elapsed > DESTROY_TIMER)
    {
        Sol_Destroy_Ent(world, id);
        return;
    }
    if (data->elapsed > REMOVE_PHYSX_TIMER)
    {
        world->masks[id] &= ~BITC(HAS_ScBody3);
        world->masks[id] &= ~BITC(HAS_ScBody2);
    }
}

void Move_Dead_Enter(World *world, int id, ScMove3 *move, ScCmd *cmd)
{
    move->targetHeight = move->baseHeight * 0.6f;
}
void Move_Dead_Exit(World *world, int id, ScMove3 *move, ScCmd *cmd)
{
    move->targetHeight = move->baseHeight;
}
bool Move_Dead_CanExit(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 next)
{
    return true;
}
bool Move_Dead_CanEnter(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 last)
{
    if (Sol_Comp_Has(world, id, ScCombat))
    {
        ScCombat *combat = Sol_Comp_Get(world, id, ScCombat);
        return combat->health == 0.0f;
    }
    return false;
}
