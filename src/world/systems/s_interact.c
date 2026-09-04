#include "world.h"
#include "sol_math.h"
#include "profiler.h"

static SolProfiler profile = {.name = "Interact"};

void Sol_Press(bool hover, bool press, InteractState *state)
{
    if (hover)
    {
        *state |= INTERACT_HOVERED;
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
    else
        *state &= ~(INTERACT_HOVERED | INTERACT_HELD);
}

void Interact_Tick(World *world, double dt)
{
    float fdt = (float)dt;
    Prof_Begin(&profile);

    SparseSet_ScInteract *set     = Sol_Comp_Set(world, ScInteract);
    SparseSet_ScCmd      *cmd_set = Sol_Comp_Set(world, ScCmd);

    for (int i = 0; i < set->cnt; i++)
    {
        int         id       = set->dense[i];
        ScInteract *interact = &set->data[i];
        ScXform    *xform    = Sol_Comp_Get(world, id, ScXform);

        interact->state &= (INTERACT_HELD | INTERACT_TOGGLED | INTERACT_TOGGLEABLE);

        bool inrange = false;
        bool holding = false;

        for (int j = 0; j < cmd_set->cnt; j++)
        {
            int cmd_id = cmd_set->dense[j];
            if (id == cmd_id)
                continue;
            ScCmd *cmd = &cmd_set->data[j];

            if (Sol_Comp_Has(world, cmd_id, ScXform))
            {
                ScXform *cmd_xform = Sol_Comp_Get(world, cmd_id, ScXform);
                if (glms_vec3_distance(xform->pos, cmd_xform->pos) < interact->range)
                {
                    inrange = true;
                    holding = cmd->actionState & BITC(ACTION_INTERACT);
                }
            }
        }

        Sol_Press(inrange, holding, &interact->state);
    }
    Prof_EndEz(&profile, true, dt);
}

void Sol_Interact_FindTopmost(World *world, vec2s point)
{
    
}