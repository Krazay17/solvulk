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
const float drag_dist2     = 100.0f;

static void Cmd_Update(World *world, SparseSet_ScInteract *set_interact)
{
    // int i, j;
    // SparseSet_ScCmd *set_cmd = Sol_Comp_Set(world, ScCmd);
    // // #pragma omp parallel for schedule(dynamic)
    // for (i = 0; i < set_cmd->cnt; i++)
    // {
    //     int id               = set_cmd->dense[i];
    //     ScCmd *cmd           = &set_cmd->data[i];
    //     vec3s pos            = Sol_Body3_GetHead(world, id);
    //     int best_id          = 0;
    //     float best_dot       = 0.0f;
    //     ScInteract *interact = NULL;
    //     for (j = set_interact->cnt - 1; j >= 0; j--)
    //     {
    //         int idB = set_interact->dense[j];
    //         if (id == idB)
    //             continue;
    //         float range  = set_interact->data[j].range;
    //         vec3s posB   = world->xform.pos[idB];
    //         float d2     = glms_vec3_distance2(posB, pos);
    //         vec3s dir_to = glms_vec3_scale(glms_vec3_sub(posB, pos), 1.0f / (d2 > 0 ? sqrt(d2) : 1.0f));
    //         float range2 = range > 0 ? range * range : 5.0f;
    //         if (d2 < range2)
    //         {
    //             float dot = glms_vec3_dot(cmd->aimdir, dir_to);
    //             if (dot > best_dot)
    //             {
    //                 best_dot = dot;
    //                 best_id  = idB;
    //             }
    //         }
    //     }
    //     if (best_id > 0)
    //     {
    //         cmd->interact = best_id;
    //     }
    // }

    // for (i = 0; i < set_cmd->cnt; i++)
    // {
    //     int id                    = set_cmd->dense[i];
    //     ScCmd *cmd                = &set_cmd->data[i];
    //     bool is_player            = Sol_Comp_Has(world, id, ScPlayer);
    //     int best_id               = cmd->interact;
    //     ScInteract *best_interact = Sol_Comp_Get(world, best_id, ScInteract);
    //     if (best_id < 1 || !best_interact)
    //         continue;

    //     if (is_player)
    //     {
    //         best_interact->state |= INTERACT_HOVERED;
    //         best_interact->is_local = true;
    //     }
    //     ScHook *hook = Sol_Comp_Get(world, best_id, ScHook);
    //     if ((cmd->actionState & BITC(ACTION_INTERACT)))
    //     {
    //         best_interact->state |= INTERACT_DOWN;

    //         best_interact->interactor = id;
    //         if (best_interact->state & INTERACT_TOGGLEABLE)
    //             best_interact->state ^= INTERACT_TOGGLED;

    //         if (!(cmd->action_state_prev & BITC(ACTION_INTERACT)))
    //         {
    //             if (hook && hook->pressed)
    //                 hook->pressed(world, best_id, id);
    //         }

    //         if (hook && hook->held)
    //             hook->held(world, best_id, id);
    //     }
    //     else if (cmd->action_state_prev & BITC(ACTION_INTERACT))
    //     {
    //         best_interact->interactor = id;
    //         if (hook && hook->release)
    //             hook->release(world, best_id, id);
    //     }
    // }
}

static void Slider_Update(World *world, SparseSet_ScInteract *set_interact)
{
    SparseSet_ScSlider *set = Sol_Comp_Set(world, ScSlider);
    for (int i = 0; i < set->cnt; i++)
    {
        int id               = set->dense[i];
        ScSlider *slider     = &set->data[i];
        ScInteract *interact = &set_interact->data[set_interact->sparse[id]];
        Xform xform          = Xform_Get(world, id);
        if (!(interact->state & (1ULL << INTERACT_DOWN)))
            continue;
        vec3s origin   = glms_vec3_add(xform.pos, slider->offset); // world-space start of track
        vec3s to_mouse = glms_vec3_sub(interact->drag_target, origin);
        float t        = glms_vec3_dot(to_mouse, slider->axis) / slider->track_len;
        t              = glm_clamp(t, 0.0f, 1.0f);

        if (slider->step > 0.0f)
            t = roundf(t / slider->step) * slider->step;

        slider->value = t;
    }
}

static inline bool Pressed(ScInteract *c)
{
}

void Interact_Update(World *world, double dt)
{
    SparseSet_ScInteract *set = Sol_Comp_Set(world, ScInteract);
    for (int i = 0; i < set->cnt; i++)
    {
        ScInteract *c = &set->data[i];
        c->state_prev = c->state;
        c->state      = (c->state & 1u << INTERACT_ACTIVE) | c->pending;
        c->pending    = 0;
        c->state_just = c->state & ~c->state_prev;
    }

    for (int i = 0; i < set->cnt; i++)
    {
        int id        = set->dense[i];
        ScInteract *c = &set->data[i];
        bool down     = c->state & 1u << INTERACT_DOWN;
        bool pressed  = c->state_just & (1u << INTERACT_DOWN);
        bool clicked  = (c->state & (1u << INTERACT_HOVERED)) && (c->state_prev & (1u << INTERACT_DOWN));
        if (clicked && c->flags & INTERACTFLAG_TOGGLEABLE)
            c->state ^= (1u << INTERACT_ACTIVE);
        forc(world, id, ScHook)
        {

            if (down && c->held)
                c->held(world, id, 0);
            if (pressed && c->pressed)
                c->pressed(world, id, 0);
            if (clicked && c->clicked)
            {
                c->clicked(world, id, 0);
            }
        }
    }

    Slider_Update(world, set);
}

void Drag_Update(World *world, double dt)
{
    float fdt = (float)dt;

    SparseSet_ScInteract *set = Sol_Comp_Set(world, ScInteract);

    for (int j = 0; j < set->cnt; j++)
    {
        int interact_id = set->dense[j];
        ScInteract *c   = &set->data[j];

        if (!(c->state & (1u << INTERACT_DRAGGING)))
            continue;

        ScBody2 *body2 = Sol_Comp_Get(world, interact_id, ScBody2);
        if (body2)
        {
            vec3s xform_pos = world->xform.pos[interact_id];
            vec3s delta     = glms_vec3_sub(c->drag_target, xform_pos);

            float k    = fminf(40.0f, 1.0f / fdt);
            body2->vel = (vec3s){delta.x * k, delta.y * k, 0.0f};
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
        int id         = set->dense[i];
        ScBody2 *body2 = Sol_Comp_Get(world, id, ScBody2);
        if (body2 && Sol_Body2_ContainsPoint(world, id, point))
        {
            // Use >= so newer/topmost elements on the same layer take priority
            if (body2->zindex >= bestZ)
            {
                bestZ = body2->zindex;
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

void Sol_Interact_Post(World *w, int id, u32 bits)
{
    if (!w || id <= 0)
        return;
    forc(w, id, ScInteract)
    {
        if (!(c->flags & INTERACTFLAG_DRAGGABLE))
            bits &= ~(1u << INTERACT_DRAGGING);
        c->pending |= bits;
    }
}
void Sol_Interact_BeginDrag(World *w, int id, vec3s point)
{
    forc(w, id, ScInteract) c->drag_offset = glms_vec3_sub(point, w->xform.pos[id]);
}
void Sol_Interact_Drag(World *w, int id, vec3s point)
{
    forc(w, id, ScInteract)
    {
        if (!(c->flags & (INTERACTFLAG_DRAGGABLE | INTERACTFLAG_ONLYDRAGGABLE)))
            return;
        c->drag_target = glms_vec3_sub(point, c->drag_offset);
        c->pending |= (1u << INTERACT_DRAGGING);
    }
}
void Sol_Interact_Down(World *w, int id, vec3s point)
{
    forc(w, id, ScInteract)
    {
        c->drag_target = point;
        c->pending |= (1u << INTERACT_DOWN);
    }
}