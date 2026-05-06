#pragma once
#include "sol/sol.h"

#include "profiler.h"
#include "camera/camera.h"
#include "sol_math.h"
#include "render/render.h"
#include "audio/audio.h"

#include "texture/texture.h"
#include "model/model.h"
#include "font/font.h"
#include "xform/xform.h"
#include "physx/physx.h"
#include "ability/ability.h"
#include "movement/movement.h"
#include "controller/controller.h"

#define SOL_TIMESTEP (1.0 / 60.0)
#define MAX_WORLDS 4

typedef void (*SystemInit)(World *);
typedef void (*SystemFunc)(World *, double, double);

typedef bool     Active;
typedef uint32_t Mask;

typedef enum
{
    SYSTEM_STEP,
    SYSTEM_TICK,
    SYSTEM_DRAW,
} SystemKind;

typedef struct
{
    SystemInit init;
    SystemFunc tick;
    SystemFunc step;
    SystemFunc draw2d;
    SystemFunc draw3d;
} SystemConfig;

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

SolLook *Sol_Input_GetLook();

SOLAPI void Sol_Begin_Draw();
SOLAPI void Sol_Begin_3D(SolCamera *cam);
SOLAPI void Sol_End_Draw();

SOLAPI void World_Step(World *world, double dt, double time);
SOLAPI void World_Tick(World *world, double dt, double time);
SOLAPI void World_Draw3d(World *world, double dt, double time);
SOLAPI void World_Draw2d(World *world, double dt, double time);

extern const char *fontResourceName[SOL_FONT_COUNT][2];
void               Sol_Load_Resources();
SolResource        Sol_LoadResource(const char *resourceName);
int                Sol_ReadFile(const char *filename, SolResource *outRes);

void Sol_MessageBox(const char *text, const char *level);
void Sol_Platform_LockCursor(bool lock);