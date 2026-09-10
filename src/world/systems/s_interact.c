/*
 * File: s_interact.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-08
 *
 */
#include "world.h"
#include "sol_math.h"
#include "sol_user.h"
#include "sol_core.h"

static const float drag_dist2 = 100.0f;

static void User_Interact(World *world, double dt)
{
    if (world->index == sol_user.focus_w && sol_user.focus > 0)
    {
        ScInteract *interact = Sol_Comp_Get(world, sol_user.focus, ScInteract);
        interact->state |= INTERACT_MOUSEHOVERED;
        interact->state |= INTERACT_DRAGGING;
        interact->state |= INTERACT_DOWN;

        if (!(interact->state_prev & INTERACT_MOUSEHOVERED))
        {
            interact->hover_start_time = world->tickTime;
        }

        ScHook *hook = Sol_Comp_Get(world, sol_user.focus, ScHook);
        if (!(interact->state_prev & INTERACT_DOWN))
        {
            interact->state |= INTERACT_JUSTDOWN;
            interact->down_start_time = world->tickTime;
            if (hook && hook->pressed)
                hook->pressed(world, sol_user.target, 0, dt, hook->data);
        }
        if (hook && hook->held)
            hook->held(world, sol_user.target, 0, dt, hook->data);
    }
    else if (world->index == sol_user.target_w && sol_user.target > 0)
    {
        ScInteract *interact = Sol_Comp_Get(world, sol_user.target, ScInteract);
        interact->state |= INTERACT_MOUSEHOVERED;

        if (!(interact->state_prev & INTERACT_MOUSEHOVERED))
        {
            interact->hover_start_time = world->tickTime;
        }
        if (interact->state_prev & INTERACT_DOWN)
        {
            interact->state |= INTERACT_JUSTUP;
            interact->down_end_time = world->tickTime;
            if (interact->state & INTERACT_TOGGLEABLE)
                interact->state ^= INTERACT_TOGGLED;

            ScHook *hook = Sol_Comp_Get(world, sol_user.target, ScHook);
            if (hook && hook->release)
                hook->release(world, sol_user.target, 0, dt, hook->data);
        }
    }
}

void Interact_Tick(World *world, double dt)
{
    SparseSet_ScInteract *set = Sol_Comp_Set(world, ScInteract);
    SparseSet_ScCmd *set_cmd  = Sol_Comp_Set(world, ScCmd);
    for (int i = 0; i < set->cnt; i++)
    {
        ScInteract *interact = &set->data[i];
        interact->state_prev = interact->state;
        interact->state &= (INTERACT_TOGGLED | INTERACT_DRAGGING | INTERACT_TOGGLEABLE);
    }
    User_Interact(world, dt);
    for (int i = 0; i < set_cmd->cnt; i++)
    {
        int id               = set_cmd->dense[i];
        ScCmd *cmd           = &set_cmd->data[i];
        bool is_player       = Sol_Comp_Has(world, id, ScPlayer);
        vec3s pos            = Sol_Body3_GetHead(world, id);
        int best_id          = 0;
        float best_dot       = 0.0f;
        ScInteract *interact = NULL;
        for (int j = 0; j < set->cnt; j++)
        {
            int idB = set->dense[j];
            if (id == idB)
                continue;
            interact     = &set->data[j];
            vec3s posB   = world->xform.pos[idB];
            float d2     = glms_vec3_distance2(posB, pos);
            vec3s dir_to = glms_vec3_scale(glms_vec3_sub(posB, pos), 1.0f / (d2 > 0 ? sqrt(d2) : 1.0f));
            float range2 = interact->range * interact->range;
            if (d2 < range2)
            {
                float dot = glms_vec3_dot(cmd->aimdir, dir_to);
                if (dot > best_dot)
                {
                    best_dot = dot;
                    best_id  = idB;
                }
            }
        }
        if (best_id > 0)
        {
            ScInteract *best_interact = Sol_Comp_Get(world, best_id, ScInteract);
            if (is_player)
            {
                best_interact->state |= INTERACT_ENTHOVERED;
                if (!(best_interact->state_prev & INTERACT_ENTHOVERED))
                    best_interact->hover_start_time = world->tickTime;
            }
            ScHook *hook = Sol_Comp_Get(world, best_id, ScHook);
            if ((cmd->actionState & BITC(ACTION_INTERACT)))
            {
                best_interact->state |= INTERACT_DOWN;
                if (!(best_interact->state_prev & INTERACT_DOWN))
                {
                    best_interact->state |= INTERACT_JUSTDOWN;
                    if (hook && hook->pressed)
                        hook->pressed(world, best_id, id, dt, hook->data);
                }

                if (hook && hook->held)
                    hook->held(world, best_id, id, dt, hook->data);
            }
            else if (best_interact->state_prev & INTERACT_DOWN)
            {
                best_interact->state |= INTERACT_JUSTUP;
                best_interact->down_end_time = world->tickTime;

                if (hook && hook->release)
                    hook->release(world, best_id, id, dt, hook->data);
            }
        }
    }
}

void Interact_Body_Step(World *world, double dt)
{
    float fdt = (float)dt;

    SparseSet_ScInteract *set_interacts = Sol_Comp_Set(world, ScInteract);

    for (int j = 0; j < set_interacts->cnt; j++)
    {
        int interact_id      = set_interacts->dense[j];
        ScInteract *interact = &set_interacts->data[j];

        if (!(interact->state & INTERACT_DRAGGING))
            continue;

        ScBody2 *body2 = Sol_Comp_Get(world, interact_id, ScBody2);
        if (body2)
        {
            vec3s xform_pos = world->xform.pos[interact_id];
            vec2s target_pos;

            // if (interact->interactors[0].id == -1)
            // { // Mouse Drag
            //     target_pos =
            //         glms_vec2_sub(sol_user.mouse_pos, (vec2s){interact->drag_offset.x, interact->drag_offset.y});
            // }
            // else if (interact->interactors > 0)
            // { // Entity Drag
            //     vec3s holder_pos = world->xform.pos[interact->active_interactor];
            //     target_pos       = glms_vec2_sub((vec2s){holder_pos.x, holder_pos.y},
            //                                      (vec2s){interact->drag_offset.x, interact->drag_offset.y});
            // }
            // else
            // {
            //     continue;
            // }

            // Apply physics velocity toward target
            // float factor = 40.0f - expf(-25.0f * fdt);
            // vec2s delta  = glms_vec2_sub(target_pos, (vec2s){xform_pos.x, xform_pos.y});
            // vec2s vel2   = glms_vec2_scale(delta, factor);

            // body2->vel = (vec3s){vel2.x, vel2.y, 0.0f};
        }
    }
}

int Sol_Interact_FindTopmost(World *world, vec2s point)
{
    int best  = 0;
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
                z             = (int)view->layer;
            }

            // Use >= so newer/topmost elements on the same layer take priority
            if (z >= bestZ)
            {
                bestZ = z;
                best  = id;
            }
        }
    }

    if (best > 0)
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