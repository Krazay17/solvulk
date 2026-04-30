#pragma once

#include "sol/types.h"

#define MAX_WORLDS 4

#define MAX_ENTS (1 << 15)
#define MAX_SYSTEMS 64

#define SYS_BIT(x) (1u << (x))

// #define SYS_LOOP(a, b)                                                                                                 \
//     u32 required = a;                                                                                                  \
//     for (int i = 0; i < b->activeCount; i++)                                                                           \
//     {                                                                                                                  \
//         u32 id = world->activeEntities[i];                                                                             \
//         if ((b->masks[id] & required) != required)                                                                     \
//             continue;

typedef struct World World;
typedef void (*SystemInit)(World *world);
typedef void (*SystemFunc)(World *world, double dt, double time);
typedef bool     Active;
typedef uint32_t Mask;

// fwds
typedef struct WorldPhysx WorldPhysx;
typedef struct WorldLines WorldLines;

typedef enum
{
    WORLD_SYS_PHYSX,
    WORLD_SYS_CONTROLLER_LOCAL,
    WORLD_SYS_CONTROLLER_AI,
    WORLD_SYS_INTERACT,
    WORLD_SYS_MODEL,
    WORLD_SYS_UI,
    WORLD_SYS_MOVEMENT,
    WORLD_SYS_LINE,
    WORLD_SYS_COMBAT,
    WORLD_SYS_BUFF,
    WORLD_SYS_VITAL,
    WORLD_SYS_SPHERE,
    WORLD_SYS_TIMER,
    WORLD_SYS_CAM,
    WORLD_SYS_PICKUP,
    WORLD_SYS_COUNT,
} WorldSystem;

typedef enum
{
    HAS_NONE          = 0,
    HAS_XFORM         = (1 << 0),
    HAS_BODY2         = (1 << 1),
    HAS_BODY3         = (1 << 2),
    HAS_TOUCH         = (1 << 3),
    HAS_SHAPE         = (1 << 4),
    HAS_INTERACT      = (1 << 5),
    HAS_MODEL         = (1 << 6),
    HAS_INFO          = (1 << 7),
    HAS_UIVIEW        = (1 << 8),
    HAS_MOVEMENT      = (1 << 9),
    HAS_CONTROLLER    = (1 << 10),
    HAS_CAMERA        = (1 << 11),
    HAS_CONTROLLER_AI = (1 << 12),
    HAS_COMBAT        = (1 << 13),
    HAS_BUFF          = (1 << 14),
    HAS_VITAL         = (1 << 15),
    HAS_SPHERE        = (1 << 16),
    HAS_TIMER         = (1 << 17),
    HAS_PICKUP        = (1 << 18),
} CompBits;

typedef enum
{
    EFLAG_PICKUPABLE = (1 << 0),
    EFLAG_PICKEDUP   = (1 << 1),
} EntFlags;

typedef enum
{
    SYSTEM_STEP,
    SYSTEM_TICK,
    SYSTEM_DRAW,
} SystemKind;

typedef enum
{
    BUFF_KNOCKBACK = (1 << 0),
    BUFF_FIRE      = (1 << 1),
    BUFF_COUNT,
} BuffKind;

typedef struct CompFlags
{
    EntFlags flags;
} CompFlags;

typedef struct CompBuff
{
    float    duration;
    BuffKind kind;
    SolHit   hit;
} CompBuff;

typedef struct CompTouch
{
    bool down, up, left, right, front, back;
} CompTouch;

typedef struct CompVital
{
    u32   maxHealth, maxEnergy, maxMana;
    u32   health, energy, mana;
    bool  doesRespawn;
    float deathTime, respawnTime;
} CompVital;

typedef struct CompCombat
{
    CombatState state;
    vec3s       attackPos, attackDir;
} CompCombat;

typedef struct CompXform
{
    vec3s   pos, lastPos, drawPos;
    versors quat, lastQuat, drawQuat;
    vec3s   scale, lastScale, drawScale;
} CompXform;

typedef struct CompShape
{
    Shape2 type;
    float  width, height;
} CompShape;

typedef struct CompCam
{
    vec3s pos, dir;
} CompCam;

typedef struct CompModel
{
    SolModelId modelId;
    SolModel  *model;
    float      yOffset;
} CompModel;

typedef struct CompInfo
{
    char name[24];

} CompInfo;

typedef struct CompPickup
{
    bool active;
} CompPickup;

typedef enum InteractState
{
    INTERACT_HOVERED  = (1 << 0),
    INTERACT_PRESSED  = (1 << 1),
    INTERACT_CLICKED  = (1 << 2),
    INTERACT_TOGGLED  = (1 << 3),
    INTERACT_ISTOGGLE = (1 << 4),
} InteractState;
typedef struct CompInteract
{
    u8       states;
    float    value;
    Callback onClick;
    Callback onHold;
} CompInteract;

typedef struct CompUiButton
{
    bool isToggle, toggle;
} CompUiButton;

typedef struct CompUiSlider
{
    float value;
    float min, max;
    bool  isDragging;
} CompUiSlider;

typedef struct CompUiView
{
    SolColor baseColor;
    SolColor textColor;
    float    fontSize;
    char     text[64];
    float    textWidth;

    float hoverAnim;
    float clickAnim;

    float    borderThickness;
    SolColor borderColor;
} CompUiView;

typedef struct CompMovement
{
    vec3s        wishdir, updir, lockdir;
    float        stateTimer;
    MoveState    moveState;
    MoveConfigId configId;
} CompMovement;

typedef struct CompController
{
    vec3s              lookdir, wishdir, aimdir, aimpos, aimHitPos;
    vec2s              wishdir2;
    PlayerActionStates actionState;
    float              yaw, pitch;
    float              zoom;
    u32                aimHitEnt;
} CompController;

typedef struct CompSphere
{
    float radius;
    vec4s color;
} CompSphere;

typedef struct CompTimer
{
    float elapsed, duration;
} CompTimer;

typedef struct CompBody
{
    vec3s  vel, impulse, force;
    Shape3 shape;
    float  grounded, airtime;
    float  radius, height, length;
    float  mass, invMass, restitution;
    u8     group;
} CompBody;

typedef struct
{
    SystemInit init;
    SystemFunc step;
    SystemFunc tick;
    SystemFunc draw;
} SystemConfig;

typedef struct World
{
    bool       worldActive;
    SystemFunc stepSystems[MAX_SYSTEMS];
    SystemFunc tickSystems[MAX_SYSTEMS];
    SystemFunc drawSystems[MAX_SYSTEMS];
    int        stepCount;
    int        tickCount;
    int        drawCount;
    u32        systemBits;

    WorldPhysx *spatial;
    WorldLines *lines;

    int playerID;
    int activeEntities[MAX_ENTS];
    int activeCount;

    Active actives[MAX_ENTS];
    Mask   masks[MAX_ENTS];

    CompFlags      flags[MAX_ENTS];
    CompTimer      timers[MAX_ENTS];
    CompSphere     spheres[MAX_ENTS];
    CompVital      vitals[MAX_ENTS];
    CompBuff       buffs[MAX_ENTS];
    CompXform      xforms[MAX_ENTS];
    CompBody       bodies[MAX_ENTS];
    CompShape      shapes[MAX_ENTS];
    CompModel      models[MAX_ENTS];
    CompInteract   interacts[MAX_ENTS];
    CompInfo       infos[MAX_ENTS];
    CompUiView     uiElements[MAX_ENTS];
    CompMovement   movements[MAX_ENTS];
    CompController controllers[MAX_ENTS];
    CompCombat     combats[MAX_ENTS];
} World;

SOLAPI World *World_Create(void);
SOLAPI World *World_Create_Default(void);
SOLAPI void   World_Destroy(World *world);
void          World_System_Add(World *world, WorldSystem system);
SOLAPI int    Sol_World_GetEntCount(World *world);

// Add thing to world
SOLAPI void Sol_World_Line_Add(World *world, vec3s a, vec3s b, vec3s color, vec3s bColor, float ttl);

// Add Prefab entity
SOLAPI int Sol_Prefab_Button(World *world, vec3s pos, const char *text);
SOLAPI int Sol_Prefab_Wizard(World *world, vec3s pos);
SOLAPI int Sol_Prefab_Boxman(World *world, vec3s pos);
int        Sol_Prefab_Ball(World *world, vec3s pos, vec3s vel, CompSphere sphere);

// Add Entity
SOLAPI int  Sol_Create_Ent(World *world);
SOLAPI void Sol_Destroy_Ent(World *world, int id);

// Add Component to entity
void            Sol_Flags_Add(World *world, int id, EntFlags flags);
CompXform      *Sol_Xform_Add(World *world, int id, vec3s pos);
CompShape      *Sol_Shape_Add(World *world, int id);
CompBody       *Sol_Body2_Add(World *world, int id);
CompInteract   *Sol_Interact_Add(World *world, int id);
CompInfo       *Sol_Info_Add(World *world, int id);
CompUiView     *Sol_UiView_Add(World *world, int id);
CompMovement   *Sol_Movement_Add(World *world, int id, CompMovement init);
CompController *Sol_ControllerLocal_Add(World *world, int id);
CompController *Sol_ControllerRemote_Add(World *world, int id);
CompController *Sol_ControllerAi_Add(World *world, int id);
CompBody       *Sol_Body_Add(World *world, int id, CompBody init);
CompCombat     *Sol_Combat_Add(World *world, int id, CompCombat init);
CompModel      *Sol_Model_Add(World *world, int id, CompModel init);
CompBuff       *Sol_Buff_Add(World *world, int id, CompBuff init);
CompSphere     *Sol_Sphere_Add(World *world, int id, CompSphere init);
CompTimer      *Sol_Timer_Add(World *world, int id, CompTimer init);

// Xform systems
SOLAPI void Xform_Snapshot(World *world);
SOLAPI void Xform_Interpolate(World *world, float alpha);
// Init Systems
SOLAPI void Physx_Init(World *world);
SOLAPI void Lines_Init(World *world);
// Step Systems
SOLAPI void Sol_System_Movement_2d_Step(World *world, double dt, double time);
SOLAPI void Sol_System_Movement_3d_Step(World *world, double dt, double time);
SOLAPI void Sol_System_Step_Physx_2d(World *world, double dt, double time);
SOLAPI void Physx_Step(World *world, double dt, double time);
SOLAPI void Buff_Step(World *world, double dt, double time);
SOLAPI void Vital_Step(World *world, double dt, double time);
void        Pickup_Step(World *world, double dt, double time);

// Tick Systems
SOLAPI void Sol_System_Info_Tick(World *world, double dt, double time);
SOLAPI void Interact2d_Tick(World *world, double dt, double time);
SOLAPI void Sol_System_Controller_Local_Tick(World *world, double dt, double time);
SOLAPI void Sol_System_Controller_Ai_Tick(World *world, double dt, double time);
SOLAPI void Sol_System_Camera_Tick(World *world, double dt, double time);
SOLAPI void Sol_System_Line_Tick(World *world, double dt, double time);
void        Timer_Tick(World *world, double dt, double time);
void        Combat_Tick(World *world, double dt, double time);

// Draw Systems
SOLAPI void Sol_System_Model_Draw(World *world, double dt, double time);
SOLAPI void Sol_System_Line_Draw(World *world, double dt, double time);
SOLAPI void UiView_Draw(World *world, double dt, double time);
// Draw Calls
SOLAPI void Sol_Draw_Model_Instanced(SolModelId handle, uint32_t instanceCount, uint32_t firstInstance);
SOLAPI void Sol_Draw_Rectangle(SolRect rect, SolColor color, float thickness);
SOLAPI void Sol_Draw_Line(SolLine *lines, int count);
void        Sol_Draw_Text(const char *str, float x, float y, float size, SolColor color, SolFontId fontId);
void        Cam_Update_3D(World *world, double dt, double time, float alpha);
void        Vital_Draw(World *world, double dt, double time);
void        Crosshair_Draw(World *world, double dt, double time);
void        Sphere_Draw(World *world, double dt, double time);

// System oneshot calls
void          Sol_Sphere_ColorAll(World *world, vec4s color);
void          Xform_Teleport(CompXform *xform, vec3s pos);
SolRayResult  Sol_ScreenRaycast(World *world, float screenX, float screenY, SolRay ray);

static SystemConfig world_systems[WORLD_SYS_COUNT] = {
    [WORLD_SYS_TIMER] = {.tick = Timer_Tick},
    [WORLD_SYS_PHYSX] = {.init = Physx_Init, .step = Physx_Step},

    [WORLD_SYS_CONTROLLER_LOCAL] = {.tick = Sol_System_Controller_Local_Tick},
    [WORLD_SYS_CONTROLLER_AI]    = {.tick = Sol_System_Controller_Ai_Tick},
    [WORLD_SYS_INTERACT]         = {.tick = Interact2d_Tick},
    [WORLD_SYS_MOVEMENT]         = {.step = Sol_System_Movement_3d_Step},
    [WORLD_SYS_COMBAT]           = {.tick = Combat_Tick},
    [WORLD_SYS_BUFF]             = {.step = Buff_Step},
    [WORLD_SYS_VITAL]            = {.step = Vital_Step, .draw = Vital_Draw},
    [WORLD_SYS_MODEL]            = {.draw = Sol_System_Model_Draw},
    [WORLD_SYS_UI]               = {.draw = UiView_Draw},
    [WORLD_SYS_LINE]             = {.init = Lines_Init, .tick = Sol_System_Line_Tick, .draw = Sol_System_Line_Draw},
    [WORLD_SYS_SPHERE]           = {.draw = Sphere_Draw},
    [WORLD_SYS_CAM]              = {.draw = Crosshair_Draw},
    [WORLD_SYS_PICKUP]           = {.step = Pickup_Step},
};
