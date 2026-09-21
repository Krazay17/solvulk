#include "world.h"

void Cmd_Update(World *world, double dt)
{
    SparseSet_ScCmd *set = Sol_Comp_Set(world, ScCmd);

    int count = set->cnt;
    for (int i = 0; i < count; i++)
    {
        int id                 = set->dense[i];
        ScCmd *cmd             = &set->data[i];
        cmd->action_state_prev = cmd->actionState;

        if (!Sol_Comp_Has(world, id, ScPlayer) && !Sol_Comp_Has(world, id, ScAi))
        {
            cmd->actionState = 0;
            cmd->wishdir     = (vec3s){0};
            cmd->wishdir2    = (vec3s){0};
            cmd->interact    = 0;
            cmd->target      = 0;
        }
    }
}