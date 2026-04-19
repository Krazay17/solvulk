#include "sol_core.h"

World *World_Create(void)
{
    World *world = calloc(1, sizeof(World));
    if (world)
    {
        world->worldActive = true;

        SolState *state = Sol_GetState();
        state->worlds[state->worldCount++] = world;
        world->playerID = -1;
        SpatialTable_Init(&world->worldSpatial.table_dynamic, SPATIAL_DYNAMIC_SIZE, SPATIAL_DYNAMIC_ENTRIES);
        SpatialTable_Init(&world->worldSpatial.table_static, SPATIAL_STATIC_SIZE, SPATIAL_STATIC_ENTRIES);
    }

    return world;
}

World *World_Create_Default(void)
{
    World *world = World_Create();
    if (world)
    {
        SystemFunc stepSystemInit[] = {
            Sol_System_Movement_2d_Step,
            Sol_System_Movement_3d_Step,
            Sol_System_Step_Physx_2d,
            Sol_System_Step_Physx_3d,
        };
        int stepSystemCount = 4;
        world->stepCount = stepSystemCount;
        memcpy(world->stepSystems, stepSystemInit, sizeof(SystemFunc) * stepSystemCount);

        SystemFunc tickSystemInit[] = {
            Sol_System_Controller_Local_Tick,
            Sol_System_Controller_Ai_Tick,
            Sol_System_Interact_Ui,
            Sol_System_Info_Tick,
            Sol_System_Camera_Tick,
        };
        int tickSystemCount = 5;
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

CompXform *Entity_Add_Xform(World *world, int id, vec3s pos)
{
    world->xforms[id] = (CompXform){
        .pos = pos,
        .scale = (vec3s){1.0f, 1.0f, 1.0f},
    };
    world->masks[id] |= HAS_XFORM;
    return &world->xforms[id];
}

CompBody *Entity_Add_Body2(World *world, int id)
{
    world->masks[id] |= HAS_BODY2;
    return &world->bodies[id];
}

CompBody *Entity_Add_Body3(World *world, int id, CompBody init_body)
{
    CompBody body = init_body;
    body.height = body.height ? body.height : 0.5f;
    body.radius = body.radius ? body.radius : 0.5f;
    body.length = body.length ? body.length : 0.5f;
    body.invMass = body.mass > 0 ? 1.0f / body.mass : 0;

    if(body.shape == SHAPE3_MOD && body.mass == 0)
    {
        spatial_static_add_model(&world->worldSpatial, world->models[id].model, &world->xforms[id]);
    }

    world->bodies[id] = body;
    world->masks[id] |= HAS_BODY3;
    return &world->bodies[id];
}

CompShape *Entity_Add_Shape(World *world, int id)
{
    world->masks[id] |= HAS_SHAPE;
    return &world->shapes[id];
}

CompInteractable *Entity_Add_Interact(World *world, int id)
{

    world->masks[id] |= HAS_INTERACT;
    return &world->interactables[id];
}

CompInfo *Entity_Add_Info(World *world, int id)
{
    world->masks[id] |= HAS_INFO;
    return &world->infos[id];
}

CompUiElement *Entity_Add_UiElement(World *world, int id)
{
    world->masks[id] |= HAS_UI_ELEMENT;
    return &world->uiElements[id];
}

CompMovement *Entity_Add_Movement(World *world, int id)
{
    world->masks[id] |= HAS_MOVEMENT;
    return &world->movements[id];
}

CompController *Entity_Add_Controller_Local(World *world, int id)
{
    world->masks[id] |= HAS_CONTROLLER;
    world->playerID = id;
    return &world->controllers[id];
}

CompController *Entity_Add_Controller_Remote(World *world, int id)
{
    world->masks[id] |= HAS_CONTROLLER;
    return &world->controllers[id];
}

CompController *Entity_Add_Controller_Ai(World *world, int id)
{
    world->masks[id] |= HAS_CONTROLLER_AI;
    return &world->controllers[id];
}

CompModel *Entity_Add_Model(World *world, int id, SolModelId model)
{
    world->models[id] = (CompModel){
        .gpuHandle = model,
        .model = Sol_GetModel(model),
    };
    world->masks[id] |= HAS_MODEL;
    return &world->models[id];
}

int Sol_World_GetEntCount(World *world)
{
    return world->activeCount;
}

CompUiElement *Entity_Get_UiElement(World *world, int id)
{
    return &world->uiElements[id];
}
