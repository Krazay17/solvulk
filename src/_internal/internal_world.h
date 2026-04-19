//internal_world.h
#pragma once

#include "internal_types.h"

typedef enum
{
    SYSTEM_STEP,
    SYSTEM_TICK,
    SYSTEM_DRAW,
} SystemKind;

typedef struct World World;
typedef void (*SystemFunc)(World *world, double dt, double time);
struct World
{
    bool worldActive;
    SystemFunc stepSystems[MAX_SYSTEMS];
    int stepCount;
    SystemFunc tickSystems[MAX_SYSTEMS];
    int tickCount;
    SystemFunc drawSystems[MAX_SYSTEMS];
    int drawCount;

    WorldSpatial worldSpatial;

    int playerID;
    int activeEntities[MAX_ENTS];
    int activeCount;

    Active actives[MAX_ENTS];
    Mask masks[MAX_ENTS];

    CompXform xforms[MAX_ENTS];
    CompBody bodies[MAX_ENTS];
    CompShape shapes[MAX_ENTS];
    CompModel models[MAX_ENTS];
    CompInteractable interactables[MAX_ENTS];
    CompInfo infos[MAX_ENTS];
    CompUiElement uiElements[MAX_ENTS];
    CompMovement movements[MAX_ENTS];
    CompController controllers[MAX_ENTS];
};

SOLAPI void World_Step(World *world, double dt, double time);
SOLAPI void World_Tick(World *world, double dt, double time);
SOLAPI void World_Draw(World *world, double dt, double time);

SOLAPI void World_System_Add(World *world, SystemFunc func, SystemKind kind);

// Xform systems
SOLAPI void Sol_System_Xform_Snapshot(World *world);
SOLAPI void Sol_System_Xform_Interpolate(World *world, float alpha);
// Step Systems
SOLAPI void Sol_System_Movement_2d_Step(World *world, double dt, double time);
SOLAPI void Sol_System_Movement_3d_Step(World *world, double dt, double time);
SOLAPI void Sol_System_Step_Physx_2d(World *world, double dt, double time);
SOLAPI void Sol_System_Step_Physx_3d(World *world, double dt, double time);
// Tick Systems
SOLAPI void Sol_System_Info_Tick(World *world, double dt, double time);
SOLAPI void Sol_System_Interact_Ui(World *world, double dt, double time);
SOLAPI void Sol_System_Controller_Local_Tick(World *world, double dt, double time);
SOLAPI void Sol_System_Controller_Ai_Tick(World *world, double dt, double time);
SOLAPI void Sol_System_Camera_Tick(World *world, double dt, double time);
// Draw Systems
SOLAPI void Sol_System_Model_Draw(World *world, double dt, double time);
SOLAPI void Sol_System_UI_Draw(World *world, double dt, double time);

// SOLAPI CompXform *Entity_Get_Xform(World *world, int id);
// SOLAPI CompBody *Entity_Get_Body2(World *world, int id);
// SOLAPI CompBody *Entity_Get_Body3(World *world, int id);
// SOLAPI CompShape *Entity_Get_Shape(World *world, int id);
// SOLAPI CompInteractable *Entity_Get_Interact(World *world, int id);
// SOLAPI CompInfo *Entity_Get_Info(World *world, int id);
// SOLAPI CompMovement *Entity_Get_Movement(World *world, int id);
// SOLAPI CompController *Entity_Get_Controller(World *world, int id);
SOLAPI CompUiElement *Entity_Get_UiElement(World *world, int id);