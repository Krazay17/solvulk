#include "world.h"
#include "sol_math.h"

static void Press(bool press, InteractState *state)
{
    if (press)
    {
        *state |= INTERACT_HELD;
    }
    else if (*state & INTERACT_HELD)
    {
        *state &= ~INTERACT_HELD;
        *state |= INTERACT_PRESSED;
        if (*state & INTERACT_TOGGLEABLE)
            *state ^= INTERACT_TOGGLED;
    }
}

void Interact_Tick(World *world, double dt)
{
    float fdt = (float)dt;

    SparseSet_ScInteract *set     = Sol_Comp_Set(world, ScInteract);
    SparseSet_ScCmd      *cmd_set = Sol_Comp_Set(world, ScCmd);

    for (int i = 0; i < set->cnt; i++)
    {
        int         id       = set->dense[i];
        ScInteract *interact = &set->data[i];
        ScXform    *xform    = Sol_Comp_Get(world, id, ScXform);

        interact->state = (interact->state & INTERACT_HELD) ? interact->state = INTERACT_HELD : 0;

        for (int j = 0; j < cmd_set->cnt; j++)
        {
            int    cmd_id = cmd_set->dense[j];
            ScCmd *cmd    = &cmd_set->data[j];

            if (Sol_Comp_Has(world, cmd_id, ScXform))
            {
                ScXform *cmd_xform = Sol_Comp_Get(world, cmd_id, ScXform);
                if (glms_vec3_distance(xform->pos, cmd_xform->pos) < interact->range)
                {
                    Press(cmd->actionState & ACTION_INTERACT, &interact->state);
                }
            }
        }
    }
}