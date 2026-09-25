#include "move3/s_move3.h"
#include "world.h"
#include "sol_math.h"

#define REMOVE_PHYSX_TIMER 2.0f
#define DESTROY_TIMER 5.0f

void Move_Dead_Update(World *world, int id, ScMove3 *move, ScCmd *cmd, float dt)
{
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
        return combat->is_dead;
    }
    return false;
}

const MoveStateFuncs move_dead_funcs = {
    .update   = Move_Dead_Update,
    .enter    = Move_Dead_Enter,
    .exit     = Move_Dead_Exit,
    .canExit  = Move_Dead_CanExit,
    .canEnter = Move_Dead_CanEnter,
};
