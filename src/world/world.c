#include <assert.h>
#include <string.h>

#include "sol_core.h"
#include "world.h"

World *World_Create(void)
{
    World *world = calloc(1, sizeof(World));
    if (world)
    {
        world->worldActive = true;
        solState.worlds[solState.worldCount] = world;
        solState.worldCount++;
    }

    return world;
}

SOLAPI World *World_Create_Default(void)
{
    World *world = World_Create();
    if (world)
    {
        SystemFunc stepSystemInit[] = {};
        int stepSystemCount = 0;
        world->stepCount = stepSystemCount;
        memcpy(world->stepSystems, stepSystemInit, sizeof(SystemFunc) * stepSystemCount);

        SystemFunc tickSystemInit[] = {
            Sol_System_Movement_2d_Step,
            Sol_System_Movement_3d_Step,
            Sol_System_Step_Physx_2d,
            Sol_System_Step_Physx_3d,
            Sol_System_Controller_Local_Tick,
            Sol_System_Controller_Ai_Tick,
            Sol_System_Interact_Ui,
            Sol_System_Button_Update,
            Sol_System_Info_Tick,
            Sol_System_Camera_Tick,
        };
        int tickSystemCount = 10;
        world->tickCount = tickSystemCount;
        memcpy(world->tickSystems, tickSystemInit, sizeof(SystemFunc) * tickSystemCount);

        SystemFunc drawSystemInit[] = {
            Sol_System_Model_Draw,
            Sol_System_UI_Draw,
        };
        int drawSystemCount = 2;
        world->drawCount = drawSystemCount;
        memcpy(world->drawSystems, drawSystemInit, sizeof(SystemFunc) * drawSystemCount);
    }
    return world;
}

void World_Destroy(World *world)
{
    if (world)
        free(world);
}

void World_Step(World *world, double dt, double time)
{
    if (!world || !world->worldActive)
        return;
    for (int i = 0; i < world->stepCount; i++)
    {
        world->stepSystems[i](world, dt, time);
    }
}

void World_Tick(World *world, double dt, double time)
{
    if (!world || !world->worldActive)
        return;
    for (int i = 0; i < world->tickCount; i++)
    {
        world->tickSystems[i](world, dt, time);
    }
}

void World_Draw(World *world, double dt, double time)
{
    if (!world || !world->worldActive)
        return;
    for (int i = 0; i < world->drawCount; i++)
    {
        world->drawSystems[i](world, dt, time);
    }
}

void World_System_Add(World *world, SystemFunc func, SystemKind kind)
{
    if (!world || !func)
        return;

    switch (kind)
    {
    case SYSTEM_STEP:
        if (world->stepCount < MAX_SYSTEMS)
            world->stepSystems[world->stepCount++] = func;
        break;
    case SYSTEM_TICK:
        if (world->tickCount < MAX_SYSTEMS)
            world->tickSystems[world->tickCount++] = func;
        break;
    case SYSTEM_DRAW:
        if (world->drawCount < MAX_SYSTEMS)
            world->drawSystems[world->drawCount++] = func;
        break;
    }
}

int Entity_Create(World *world)
{
    for (int i = 0; i < MAX_ENTS; i++)
    {
        if (!world->actives[i])
        {
            world->actives[i] = true;
            world->masks[i] = HAS_NONE;

            // NEW: Track this ID in our dense list
            world->activeEntities[world->activeCount] = i;
            world->activeCount++;

            return i;
        }
    }
    return -1;
}

void Entity_Destroy(World *world, int id)
{
    // 1. Mark as inactive
    world->actives[id] = false;
    world->masks[id] = 0;

    // 2. Remove from dense list by "Swapping with Last"
    // This keeps the array dense without needing to shift everything
    for (int i = 0; i < world->activeCount; i++)
    {
        if (world->activeEntities[i] == id)
        {
            // Take the very last ID in the list and put it here
            world->activeEntities[i] = world->activeEntities[world->activeCount - 1];
            world->activeCount--;
            break;
        }
    }
}

void Entity_Add_Xform(World *world, int id, CompXform xform)
{
    world->xforms[id] = xform;
    world->masks[id] |= HAS_XFORM;
}

void Entity_Add_Body2(World *world, int id, CompBody body)
{
    world->bodies[id] = body;
    world->masks[id] |= HAS_BODY2;
}

void Entity_Add_Body3(World *world, int id, CompBody body)
{
    world->bodies[id] = body;
    world->masks[id] |= HAS_BODY3;
}

void Entity_Add_Shape(World *world, int id, CompShape shape)
{
    world->shapes[id] = shape;
    world->masks[id] |= HAS_SHAPE;
}

void Entity_Add_Interact(World *world, int id, CompInteractable interact)
{
    world->interactables[id] = interact;
    world->masks[id] |= HAS_INTERACT;
}

void Entity_Add_Info(World *world, int id, CompInfo info)
{
    world->infos[id] = info;
    world->masks[id] |= HAS_INFO;
}

void Entity_Add_UiElement(World *world, int id, CompUiElement uiElement)
{
    world->uiElements[id] = uiElement;
    world->masks[id] |= HAS_UI_ELEMENT;
}

void Entity_Add_Movement(World *world, int id, CompMovement movement)
{
    world->movements[id] = movement;
    world->masks[id] |= HAS_MOVEMENT;
}

void Entity_Add_Controller_Local(World *world, int id, CompController controller)
{
    world->controllers[id] = controller;
    world->masks[id] |= HAS_CONTROLLER;
}

SOLAPI void Entity_Add_Controller_Remote(World *world, int id, CompController controller)
{
}

SOLAPI void Entity_Add_Controller_Ai(World *world, int id, CompController controller)
{
}

SOLAPI void Entity_Add_Model(World *world, int id, CompModel model)
{
    world->models[id] = model;
    world->masks[id] |= HAS_MODEL;
}

CompUiElement Entity_Get_UiElement(World *world, int id)
{
    return world->uiElements[id];
}
