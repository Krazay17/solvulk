#include "world.h"
#include "sol_math.h"
#include "sol_user.h"
#include "sol_core.h"

const float g_drag_dist = 0.1f;

static void Press(World *world, ScInteract *interact, bool hover, bool holding, vec2s press_pos, vec2s xform2)
{
    if (hover)
    {
        interact->state |= INTERACT_HOVERED;
        interact->hover_start_time = world->tickTime;
    }
    else
    {
        interact->state &= ~INTERACT_HOVERED;
        interact->unhover_start_time = world->tickTime;
    }

    if (holding)
    {
        if (!(interact->state & INTERACT_HELD))
        {
            interact->press_start_time = world->tickTime;
            interact->press_pos        = press_pos;
            interact->drag_offset      = glms_vec2_sub(press_pos, xform2);
        }
        interact->state |= INTERACT_HELD;
        if ((interact->state & INTERACT_DRAGGABLE) && glms_vec2_distance(interact->press_pos, press_pos) > g_drag_dist)
            interact->state |= INTERACT_DRAGGING;
    }
    else
    {
        if ((interact->state & INTERACT_HELD) && !(interact->state & INTERACT_DRAGGING) && hover)
        {
            interact->state |= INTERACT_PRESSED;
            if (interact->state & INTERACT_TOGGLEABLE)
                interact->state ^= INTERACT_TOGGLED;
        }
        interact->state &= ~(INTERACT_HELD | INTERACT_DRAGGING);
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

        interact->state &=
            (INTERACT_HELD | INTERACT_TOGGLED | INTERACT_TOGGLEABLE | INTERACT_DRAGGING | INTERACT_DRAGGABLE);

        bool  is_hovered = (sol_user.mouse_hover_worldidx == world->index && sol_user.mouse_hover_ent == id);
        bool  is_holding = false;
        vec2s hold_pos   = {0};

        if (sol_user.mouse_focus_ent >= 0)
        {
            is_holding = (sol_user.mouse_focus_ent == id && sol_user.mouse_focus_worldidx == world->index &&
                          sol_user.mouse_interact);
            hold_pos   = sol_user.mouse_pos;
        }
        else if (is_hovered)
        {
            is_holding = sol_user.mouse_interact;
            hold_pos   = sol_user.mouse_pos;
        }

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
                    is_hovered = true;
                    if (cmd->actionState & BITC(ACTION_INTERACT))
                        is_holding = true;
                }
            }
        }
        Press(world, interact, is_hovered, is_holding, hold_pos, (vec2s){xform->pos.x, xform->pos.y});
    }

    float factor = 40.0f - expf(-25.0f * fdt);
    for (int i = 0; i < set->cnt; i++)
    {
        int         id       = set->dense[i];
        ScInteract *interact = &set->data[i];

        if (interact->state & INTERACT_DRAGGING)
        {
            if (Sol_Comp_Has(world, id, ScBody2) && Sol_Comp_Has(world, id, ScXform))
            {
                ScBody2 *body   = Sol_Comp_Get(world, id, ScBody2);
                ScXform *xform  = Sol_Comp_Get(world, id, ScXform);
                vec2s    xform2 = {xform->pos.x, xform->pos.y};

                vec2s target = glms_vec2_sub(interact->drag_target, interact->drag_offset);
                vec2s delta  = glms_vec2_sub(target, xform2);
                vec2s vel2   = glms_vec2_scale(delta, factor);

                body->vel = (vec3s){vel2.x, vel2.y, 0};
            }
        }
    }
}

int Sol_Interact_FindTopmost(World *world, vec2s point)
{
    int best  = -1;
    int bestZ = -1;

    SparseSet_ScInteract *set = Sol_Comp_Set(world, ScInteract);
    for (int i = 0; i < set->cnt; i++)
    {
        int id = set->dense[i];

        if (Sol_Comp_Has(world, id, ScBody2) && Sol_Body2_ContainsPoint(world, id, point))
        {
            // Default to layer 0 for interactables without an explicit View2 component
            int z = 0;

            if (Sol_Comp_Has(world, id, ScView2))
            {
                ScView2 *view = Sol_Comp_Get(world, id, ScView2);
                z = (int)view->layer;
            }

            // Use >= so newer/topmost elements on the same layer take priority
            if (z >= bestZ)
            {
                bestZ = z;
                best  = id;
            }
        }
    }

    if (best >= 0)
        return best;

    // TODO Screencast into entities
    for (int i = 0; i < set->cnt; i++)
    {
        int id = set->dense[i];
        if (Sol_Comp_Has(world, id, ScBody3))
        {
            // Raycast logic
        }
    }

    return best;
}