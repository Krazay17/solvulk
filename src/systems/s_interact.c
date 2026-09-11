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
#include <omp.h>

#include "profiler.h"

static SolProfiler profile = {.name = "Interact"};

static void User_Update(World *world, double dt)
{
    if (world->index == sol_user.focus_w && sol_user.focus > 0)
    {
        int id               = sol_user.focus;
        ScInteract *interact = Sol_Comp_Get(world, sol_user.focus, ScInteract);
        if (!interact)
            return;

        if ((interact->state & INTERACT_DRAGGABLE) && sol_user.grab)
        {
            interact->state |= INTERACT_DRAGGING;
            if (!(interact->state_prev & INTERACT_DRAGGING))
            {
                vec3s mouse3          = (vec3s){sol_user.mouse_pos.x, sol_user.mouse_pos.y, 0};
                interact->drag_offset = glms_vec3_sub(mouse3, world->xform.pos[sol_user.focus]);
            }
            return;
        }

        interact->state |= INTERACT_DOWN;
        interact->state |= INTERACT_HOVERED;
        interact->is_local = true;

        ScHook *hook = Sol_Comp_Get(world, sol_user.focus, ScHook);
        if (!(interact->state_prev & INTERACT_DOWN))
        {
            if (hook && hook->pressed)
                hook->pressed(world, sol_user.target, sol_user.view_ent, dt, hook->data);
        }

        if (hook && hook->held)
            hook->held(world, sol_user.target, sol_user.view_ent, dt, hook->data);
    }
    else if (world->index == sol_user.target_w && sol_user.target > 0)
    {
        ScInteract *interact = Sol_Comp_Get(world, sol_user.target, ScInteract);
        if (!interact)
            return;

        interact->state |= INTERACT_HOVERED;
        interact->is_local = true;
        if (sol_user.interact_last && !(interact->state_prev & INTERACT_DRAGGING))
        {
            if (interact->state & INTERACT_TOGGLEABLE)
                interact->state ^= INTERACT_TOGGLED;
            ScHook *hook = Sol_Comp_Get(world, sol_user.target, ScHook);
            if (hook && hook->release)
                hook->release(world, sol_user.target, sol_user.view_ent, dt, hook->data);
        }
    }
}

static void Slider_Update(World *world, double dt, SparseSet_ScInteract *set_interact)
{
    SparseSet_ScSlider *set = Sol_Comp_Set(world, ScSlider);
    for (int i = 0; i < set->cnt; i++)
    {
        int id               = set->dense[i];
        ScSlider *slider     = &set->data[i];
        ScInteract *interact = &set_interact->data[set_interact->sparse[id]];
        Xform xform          = Xform_Get(world, id);
        if (!(interact->state & (INTERACT_DOWN)))
            continue;
        vec3s origin   = glms_vec3_add(xform.pos, slider->offset); // world-space start of track
        vec3s to_mouse = glms_vec3_sub((vec3s){sol_user.mouse_pos.x, sol_user.mouse_pos.y, 0}, origin);
        float t        = glms_vec3_dot(to_mouse, slider->axis) / slider->track_len;
        t              = glm_clamp(t, 0.0f, 1.0f);

        if (slider->step > 0.0f)
            t = roundf(t / slider->step) * slider->step;

        slider->value = t;
    }
}

static void Cmd_Update(World *world, double dt, SparseSet_ScInteract *set_interact)
{
    int i, j;
    SparseSet_ScCmd *set_cmd = Sol_Comp_Set(world, ScCmd);
#pragma omp parallel for schedule(dynamic)
    for (i = 0; i < set_cmd->cnt; i++)
    {
        int id               = set_cmd->dense[i];
        ScCmd *cmd           = &set_cmd->data[i];
        vec3s pos            = Sol_Body3_GetHead(world, id);
        int best_id          = 0;
        float best_dot       = 0.0f;
        ScInteract *interact = NULL;
        for (j = 0; j < set_interact->cnt; j++)
        {
            int idB = set_interact->dense[j];
            if (id == idB)
                continue;
            float range  = set_interact->data[j].range;
            vec3s posB   = world->xform.pos[idB];
            float d2     = glms_vec3_distance2(posB, pos);
            vec3s dir_to = glms_vec3_scale(glms_vec3_sub(posB, pos), 1.0f / (d2 > 0 ? sqrt(d2) : 1.0f));
            float range2 = range > 0 ? range * range : 5.0f;
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
            cmd->interact = best_id;
        }
    }

    for (i = 0; i < set_cmd->cnt; i++)
    {
        int id                    = set_cmd->dense[i];
        ScCmd *cmd                = &set_cmd->data[i];
        bool is_player            = Sol_Comp_Has(world, id, ScPlayer);
        int best_id               = cmd->interact;
        ScInteract *best_interact = Sol_Comp_Get(world, best_id, ScInteract);
        if (best_id < 1 || !best_interact)
            continue;

        if (is_player)
        {
            best_interact->state |= INTERACT_HOVERED;
            best_interact->is_local = true;
        }
        ScHook *hook = Sol_Comp_Get(world, best_id, ScHook);
        if ((cmd->actionState & BITC(ACTION_INTERACT)))
        {
            best_interact->state |= INTERACT_DOWN;

            best_interact->interactor = id;
            if (best_interact->state & INTERACT_TOGGLEABLE)
                best_interact->state ^= INTERACT_TOGGLED;

            if (!(cmd->action_state_prev & BITC(ACTION_INTERACT)))
            {
                if (hook && hook->pressed)
                    hook->pressed(world, best_id, id, dt, hook->data);
            }

            if (hook && hook->held)
                hook->held(world, best_id, id, dt, hook->data);
        }
        else if (cmd->action_state_prev & BITC(ACTION_INTERACT))
        {
            best_interact->interactor = id;
            if (hook && hook->release)
                hook->release(world, best_id, id, dt, hook->data);
        }
    }
}

static void Interact_Final(World *world, double dt, SparseSet_ScInteract *set)
{
    for (int i = 0; i < set->cnt; i++)
    {
        ScInteract *interact = &set->data[i];
        if ((interact->state & INTERACT_HOVERED))
        {
            if (!(interact->state_prev & INTERACT_HOVERED))
            {
                interact->state |= INTERACT_JUSTHOVERED;
                interact->hover_start_time = world->tickTime;
            }
        }
        else if (interact->state_prev & INTERACT_HOVERED)
        {
            interact->state |= INTERACT_JUSTUNHOVERED;
            interact->hover_end_time = world->tickTime;
        }

        if ((interact->state & INTERACT_DOWN))
        {
            if (!(interact->state_prev & INTERACT_DOWN))
            {
                interact->state |= INTERACT_JUSTDOWN;
                interact->down_start_time = world->tickTime;
            }
        }
        else if ((interact->state_prev & INTERACT_DOWN))
        {
            interact->state |= INTERACT_JUSTUP;
            interact->down_end_time = world->tickTime;
        }
    }
}

void Interact_Update(World *world, double dt)
{
    Prof_Begin(&profile);
    int i;

    SparseSet_ScInteract *set = Sol_Comp_Set(world, ScInteract);
    for (i = 0; i < set->cnt; i++)
    {
        ScInteract *interact = &set->data[i];
        interact->state_prev = interact->state;
        interact->state &= (INTERACT_TOGGLED | INTERACT_TOGGLEABLE | INTERACT_DRAGGABLE);
    }

    User_Update(world, dt);
    Slider_Update(world, dt, set);
    Cmd_Update(world, dt, set);

    Interact_Final(world, dt, set);

    Prof_EndEz(&profile, true, dt);
}

void Interact_Body_Step(World *world, double dt)
{
    float fdt = (float)dt;

    SparseSet_ScInteract *set = Sol_Comp_Set(world, ScInteract);

    for (int j = 0; j < set->cnt; j++)
    {
        int interact_id      = set->dense[j];
        ScInteract *interact = &set->data[j];

        if (!(interact->state & INTERACT_DRAGGING))
            continue;

        ScBody2 *body2 = Sol_Comp_Get(world, interact_id, ScBody2);
        if (body2)
        {
            vec3s xform_pos = world->xform.pos[interact_id];
            vec2s target_pos =
                glms_vec2_sub(sol_user.mouse_pos, (vec2s){interact->drag_offset.x, interact->drag_offset.y});
            float factor = 40.0f - expf(-25.0f * fdt);
            vec2s delta  = glms_vec2_sub(target_pos, (vec2s){xform_pos.x, xform_pos.y});
            vec2s vel2   = glms_vec2_scale(delta, factor);

            body2->vel = (vec3s){vel2.x, vel2.y, 0.0f};
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