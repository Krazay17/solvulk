/*
 * File: s_interact.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * Interact!
 */
#include "sol_core.h"

static u32 movingId;

void Sol_Interact_Init(World *world)
{
    u32 idx                 = world->tickCount++;
    world->tickSystems[idx] = System_Interact_Tick;

    world->interacts = calloc(MAX_ENTS, sizeof(CompInteract));
}

void Sol_Interact_Add(World *world, int id)
{
    CompInteract interact = {0};
    world->interacts[id]  = interact;
    world->masks[id] |= HAS_INTERACT;
}

void Sol_Interact_Set(World *world, int id, CompInteract desc)
{
    world->interacts[id] = desc;
    world->masks[id] |= HAS_INTERACT;
}

void System_Interact_Tick(World *world, double dt, double time)
{
    SolMouse mouse = Sol_Input_GetMouse();

    if (!mouse.buttons[SOL_MOUSE_MIDDLE])
        movingId = 0;

    int rayId = -1;
    if (world->systemBits & SYS_BIT(WORLD_SYS_PHYSX))
    {
        SolRayResult screenRay = {0};
        if (!mouse.locked)
            screenRay = Sol_ScreenRaycast(world, mouse.x, mouse.y, (SolRay){.mask = 0b11, .dist = 100.0f});
        if (screenRay.hit && screenRay.entId)
            rayId = screenRay.entId;

        Sol_Debug_Add("SelectedEnt", rayId);
    }
    int topId = 0;
    int topZ  = INT_MIN;

    int required = HAS_INTERACT;
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;
        bool collision = false;

        if (world->masks[id] & HAS_BODY3)
        {
            if (rayId == id)
                collision = true;
        }
        else if (world->masks[id] & HAS_BODY2)
        {
            collision      = Sol_Check_2d_Collision(Sol_Input_GetMouseUI(), (vec4s){
                                                                  Sol_Xform_GetPos(world, id).x,
                                                                  Sol_Xform_GetPos(world, id).y,
                                                                  Sol_Body2d_GetDims(world, id).x,
                                                                  Sol_Body2d_GetDims(world, id).y,
                                                              });
        }
        if (collision)
        {
            int z = 0;
            if (world->masks[id] & HAS_VIEW2D)
                z = world->view2d[id].zindex;
            if (z > topZ)
            {
                topZ  = z;
                topId = id;
            }
        }
    }

    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;

        CompInteract *interact   = &world->interacts[id];
        bool          wasPressed = interact->state & INTERACT_PRESSED;
        interact->state &= ~INTERACT_PRESSED;
        interact->state &= ~INTERACT_CLICKED;
        interact->state &= ~INTERACT_HOVERED;
        CompBody2d *body = &world->body2d[id];
        if (id != movingId)
        {
            interact->state &= ~INTERACT_MOVING;
            if (world->masks[id] & HAS_PARENT)
            {
                if (body->overlapCount)
                {
                    Sol_Parent_SetActive(world, id, true);
                    Sol_Parent_SetWithOffset(world, id, body->overlapping[0]);
                }
            }
        }

        if (id != topId)
            continue;

        interact->state |= INTERACT_HOVERED;
        if (mouse.buttons[SOL_MOUSE_MIDDLE] && !movingId)
        {
            interact->state |= INTERACT_MOVING;
            movingId = id;
            interact->grabOffset = glms_vec2_sub(Sol_Input_GetMouseUI(), (vec2s){world->xforms[id].pos.x, world->xforms[id].pos.y});
            if (world->masks[id] & HAS_PARENT)
                Sol_Parent_SetActive(world, id, false);
        }

        if (mouse.buttons[SOL_MOUSE_LEFT])
        {
            interact->state |= INTERACT_PRESSED;

            if (interact->onHold.callbackFunc)
                interact->onHold.callbackFunc(interact->state, interact->onHold.callbackData);
        }
        else if (wasPressed)
        {
            interact->state |= INTERACT_CLICKED;
            interact->state &= ~INTERACT_PRESSED;
            if (interact->state & INTERACT_TOGGLEABLE)
                interact->state ^= INTERACT_TOGGLED;

            if (interact->onClick.callbackFunc)
                interact->onClick.callbackFunc(interact->state, interact->onClick.callbackData);
        }
    }
}

InteractState Sol_Interact_GetState(World *world, int id)
{
    // Self first
    if (world->masks[id] & HAS_INTERACT)
        return world->interacts[id].state;

    // Fall back to parent
    if (world->masks[id] & HAS_PARENT)
    {
        int parentId = world->parents[id].parentId;
        if (world->masks[parentId] & HAS_INTERACT)
            return world->interacts[parentId].state;
    }

    // Neither — no state
    return 0;
}

bool Sol_Interact_GetToggle(World *world, int id)
{
    CompInteract *interact = &world->interacts[id];
    return interact->state & INTERACT_TOGGLED;
}