/*
 * File: sol.h
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * ==== Sol Blade ====
 */

#pragma once
#include "sol/sol.h"

#include "camera.h"
#include "profiler.h"
#include "resource.h"
#include "sol_math.h"

#include "audio/audio.h"
#include "ability/ability.h"
#include "font/font.h"
#include "model/model.h"
#include "movement/movement.h"
#include "physx/physx.h"
#include "render/render.h"
#include "render/render_internal.h"
#include "texture/texture.h"
#include "xform/xform.h"

#define SOL_TIMESTEP (1.0 / 60.0)
#define MAX_WORLDS 4

typedef void (*SystemInit)(World *);
typedef void (*SystemFunc)(World *, double, double);

typedef bool     Active;
typedef uint32_t Mask;
typedef struct World
{
    SystemFunc stepSystems[MAX_SYSTEMS];
    SystemFunc tickSystems[MAX_SYSTEMS];
    SystemFunc draw3dSystems[MAX_SYSTEMS];
    SystemFunc draw2dSystems[MAX_SYSTEMS];

    int       activeEntities[MAX_ENTS];
    Active    actives[MAX_ENTS];
    Mask      masks[MAX_ENTS];
    CompFlags flags[MAX_ENTS];

    CompAudio      *audios;
    CompTimer      *timers;
    SolEvents      *events;
    CompXform      *xforms;
    CompBody       *bodies;
    CompMovement   *movements;
    CompModel      *models;
    CompUi         *uiElements;
    CompInteract   *interacts;
    CompSphere     *spheres;
    CompVital      *vitals;
    CompController *controllers;
    CompBuff       *buffs;
    CompAbility    *abilities;
    WorldPhysx     *spatial;
    WorldLines     *lines;
    SolEmitters    *emitters;

    bool worldActive;

    int stepCount;
    int tickCount;
    int draw2dCount;
    int draw3dCount;
    int activeCount;
    int playerID;
    u32 systemBits;
} World;

typedef struct SolState
{
    volatile bool isRunning;
    volatile bool needsResize;
    float         windowWidth, windowHeight;
    void         *g_hwnd;
    double        gameTime;
    double        fps;
    bool          debug;
    u32           tickCounter, stepCounter;

    World   *worlds[MAX_WORLDS];
    uint16_t worldCount;
} SolState;

void Sol_MessageBox(const char *text, const char *level);
void Sol_Platform_LockCursor(bool lock);

SOLAPI void Sol_Begin_Draw();
SOLAPI void Sol_End_Draw();

SOLAPI void World_Step(World *world, double dt, double time);
SOLAPI void World_Tick(World *world, double dt, double time);
SOLAPI void World_Draw3d(World *world, double dt, double time);
SOLAPI void World_Draw2d(World *world, double dt, double time);

SolCamera *Sol_GetCamera();
