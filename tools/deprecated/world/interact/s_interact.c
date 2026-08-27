/*
 * File: s_interact.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * Interact!
 */
#include "si_interact.h"
#include "sol_core.h"
#include "world.h"
#include "sol_math.h"
#include "input.h"
#include "render/render.h"

#include "xform/s_xform.h"
#include "physx/s_body.h"
#include "physx/s_body2d.h"
#include "view/s_view2d.h"
#include "parent/s_parent.h"
#include "item/s_item.h"
#include "ability/s_ability.h"
#include "buff/s_buff.h"
#include "s_interact.h"

typedef struct InteractingEnt
{
    int    id;
    int    movingId;
    World *world;
} InteractingEnt;

static InteractingEnt interactingEnt;
static int            topmost_required = BITC(HAS_INTERACT);

static void Interact_Tick(World *world, double dt, double time)
{
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if (!WHas(world, id, BITC(HAS_INTERACT)))
            continue;
        CompInteract *interact = &world->interacts[id];

        if (interact->state & INTERACT_PRESSED)
        {
            if (INTERACT_PRESSED)
                interact->state ^= INTERACT_PRESSED;

            if (interact->onHold.callbackFunc)
                interact->onHold.callbackFunc(interact->onHold.flag, interact->onHold.callbackData);
        }
        if (interact->state & INTERACT_CLICKED)
        {
            if (interact->state & INTERACT_TOGGLEABLE)
                interact->state ^= INTERACT_TOGGLED;

            if (interact->onClick.callbackFunc)
                interact->onClick.callbackFunc(interact->onClick.flag, interact->onClick.callbackData);
        }
    }
}

static void Interact_Step(World *world, double dt, double time)
{
    float       fdt       = (float)dt;
    const float stiffness = 80.0f;
    float       alpha     = 1.0f - expf(-stiffness * fdt);

    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if (!WHas(world, id, BITC(HAS_INTERACT)))
            continue;
        CompXform    *xform    = &world->xforms[id];
        CompInteract *interact = &world->interacts[id];
        if (!(interact->state & INTERACT_DRAGGING))
            continue;
        if (WHasB(world, id, HAS_BODY2))
        {
            vec2s grabPos =
                glms_vec2_add((vec2s){xform->pos.x, xform->pos.y}, (vec2s){interact->offset.x, interact->offset.y});
            vec2s targetPos = glms_vec2_sub((vec2s){interact->targetPos.x, interact->targetPos.y}, grabPos);
            Sol_Body2d_SetVel(world, id, glms_vec2_scale(targetPos, alpha));
        }
        else if (WHasB(world, id, HAS_BODY3))
        {
            vec3s grabPos   = vecAdd(xform->pos, interact->offset);
            vec3s targetPos = vecSub(interact->targetPos, grabPos);
            Sol_Physx_SetVel(world, id, glms_vec3_scale(targetPos, alpha));
        }
    }
}

void Sol_Interact_Init(World *world)
{
    world->interacts = calloc(MAX_ENTS, sizeof(CompInteract));
    world->tooltips  = calloc(MAX_ENTS, sizeof(CompTooltip));

    WAddTick(world) = Interact_Tick;
    WAddStep(world) = Interact_Step;
    WAddStep(world) = Pickup_Step;
}

CompInteract *Sol_Interact_Add(World *world, int id)
{
    CompInteract  new      = {0};
    CompInteract *interact = &world->interacts[id];
    *interact              = new;
    WAddComp(world, id, HAS_INTERACT);
    return interact;
}

CompInteract *Sol_Interact_Get(World *world, int id)
{
    return &world->interacts[id];
}
CompTooltip *Sol_Tooltip_Add(World *world, int id, TooltipKind kind)
{
    CompTooltip  new     = {0};
    CompTooltip *tooltip = &world->tooltips[id];
    *tooltip             = new;
    WAddComp(world, id, HAS_TOOLTIP);
    return tooltip;
}

void Sol_Interact_Set(World *world, int id, CompInteract desc)
{
    CompInteract *interact = &world->interacts[id];
    *interact              = desc;
    WAddComp(world, id, HAS_INTERACT);
}

InteractState Sol_Interact_GetState(World *world, int id)
{
    // Self first
    if (world->masks[id] & BITC(HAS_INTERACT))
        return world->interacts[id].state;

    // Fall back to parent
    if (world->masks[id] & BITC(HAS_PARENT))
    {
        int parentId = world->parents[id].parentId;
        if (world->masks[parentId] & BITC(HAS_INTERACT))
            return world->interacts[parentId].state;
    }

    // Neither — no state
    return 0;
}

void Sol_Interact_AddState(World *world, int id, InteractState state)
{
    CompInteract *interact = &world->interacts[id];
    InteractState newState = interact->state | state;

    if (!(interact->state & INTERACT_HOVERED) && (state & INTERACT_HOVERED))
    {
        interact->hover_start_time = solState.gameTime;
    }

    if (!(interact->state & INTERACT_PRESSED) && (state & INTERACT_PRESSED))
    {
        interact->press_start_time = solState.gameTime;
    }

    interact->state = newState;
}

void Sol_Interact_RemState(World *world, int id, InteractState state)
{
    CompInteract *interact = &world->interacts[id];

    if ((interact->state & INTERACT_HOVERED) && (state & INTERACT_HOVERED))
    {
        interact->unhover_start_time = solState.gameTime;
    }

    interact->state &= ~state;
}

bool Sol_Interact_GetToggle(World *world, int id)
{
    CompInteract *interact = &world->interacts[id];
    return interact->state & INTERACT_TOGGLED;
}

int Sol_Interact_GetTopmost(World *world)
{
    int topZ     = -1;
    int winnerId = -1;

    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if (id == 0)
            continue;
        if (!WHas(world, id, topmost_required))
            continue;
        if (WHas(world, id, BITC(HAS_BODY2)))
        {
            vec4s bounds = {
                Sol_Xform_GetPos(world, id).x,
                Sol_Xform_GetPos(world, id).y,
                Sol_Body2d_GetDims(world, id).x,
                Sol_Body2d_GetDims(world, id).y,
            };
            if (Sol_Check_2d_Collision(Sol_Input_GetMouseUI(), bounds))
            {
                int z = world->view2d[id].zindex;
                if (z > topZ)
                {
                    topZ     = z;
                    winnerId = id;
                }
            }
        }
    }
    if (winnerId != -1)
        return winnerId;
    if (world->systemBits & BITC(WORLD_SYS_PHYSX))
    {
        SolRayResult result =
            Sol_ScreenRaycast(world, Sol_Input_GetMouse().x, Sol_Input_GetMouse().y, (SolRay){.dist = 15.0f});
        if (result.hit && WHas(world, result.entId, topmost_required))
        {
            return result.entId;
        }
    }
    return -1;
}

void Sol_Interact_DragEntityTo(World *world, int id, vec3s targetPos)
{
    CompInteract *interact = &world->interacts[id];
    if (!(interact->state & INTERACT_DRAGGING))
    {
        interact->state |= INTERACT_DRAGGING;
        interact->offset = vecSub(targetPos, Sol_Xform_GetPos(world, id));

        if (WHas(world, id, BITC(HAS_PARENT)))
        {
            Sol_Parent_SetActive(world, id, false);
        }
    }

    interact->targetPos = targetPos;
}

void Sol_Interact_EndDrag(World *world, int id)
{
    CompInteract *interact = &world->interacts[id];
    interact->state &= ~INTERACT_DRAGGING;

    // Handle parenting / drop logic on release
    // if (WHas(world, id, BITC(HAS_BODY2)) && WHas(world, id, BITC(HAS_PARENT)))
    // {
    //     CompBody2d *body = &world->body2d[id];
    //     if (body->overlapCount > 0)
    //     {
    //         Sol_Parent_SetActive(world, id, true);
    //         Sol_Parent_SetWithOffset(world, id, body->overlapping[0]);
    //     }
    // }
}

float Sol_Interact_GetHoverWeight(const CompInteract *interact, double currentTime, float duration)
{
    if (duration <= 0.0001f)
        return interact->state & INTERACT_HOVERED ? 1.0f : 0.0f;

    if (interact->state & INTERACT_HOVERED)
    {
        // Fading IN
        double elapsed = currentTime - interact->hover_start_time;
        float  t       = (float)(elapsed / duration);
        return t > 1.0f ? 1.0f : t;
    }
    else
    {
        // Fading OUT
        double elapsed = currentTime - interact->unhover_start_time;
        float  t       = 1.0f - (float)(elapsed / duration);
        return t < 0.0f ? 0.0f : t;
    }
}