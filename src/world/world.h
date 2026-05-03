#pragma once
#include "sol/base.h"

#define MAX_SYSTEMS 64
#define MAX_ENTS (1 << 15)
#define SYS_BIT(x) (1u << (x))

typedef struct World World;
typedef void (*SystemInit)(World *);
typedef void (*SystemFunc)(World *, double, double);

typedef bool     Active;
typedef uint32_t Mask;

// fwds
typedef struct WorldPhysx  WorldPhysx;
typedef struct WorldLines  WorldLines;
typedef struct SolEmitters SolEmitters;
typedef struct SolEvents   SolEvents;

typedef enum
{
    WORLD_SYS_TIMER,
    WORLD_SYS_PHYSX,

    WORLD_SYS_MOVEMENT,
    WORLD_SYS_INTERACT,
    WORLD_SYS_PICKUP,
    WORLD_SYS_COMBAT,
    WORLD_SYS_BUFF,
    WORLD_SYS_VITAL,

    WORLD_SYS_CONTROLLER_LOCAL,
    WORLD_SYS_CONTROLLER_AI,

    WORLD_SYS_MODEL,
    WORLD_SYS_LINE,
    WORLD_SYS_EMITTER,
    WORLD_SYS_SPHERE,
    WORLD_SYS_CAM,
    WORLD_SYS_UI,
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
    HAS_EVENT         = (1 << 19),
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

typedef enum AbilityState
{
    ABILITY_CLAW,
    ABILITY_COUNT,
} AbilityState;

typedef struct CompAbility
{
    AbilityState state;
    vec3s        attackPos, attackDir;
    float        cooldown;
} CompAbility;
typedef struct
{
    SystemInit init;
    SystemFunc tick;
    SystemFunc step;
    SystemFunc draw2d;
    SystemFunc draw3d;
} SystemConfig;

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
    i32        currentAnim, lastAnim;
    float      currentSeek, lastSeek;
    float      blendFactor, blendSpeed;
    mat4       bones[MAX_BONES];
} CompModel;

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
    vec4s baseColor;
    vec4s textColor;
    float fontSize;
    char  text[64];
    float textWidth;

    float hoverAnim;
    float clickAnim;

    float borderThickness;
    vec4s borderColor;
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
    vec3s  vel, impulse, force, groundNormal;
    vec3s  gravity;
    float  grounded, airtime;
    float  radius, height, length;
    float  mass, invMass, restitution;
    Shape3 shape;
    u8     group;
} CompBody;

typedef struct World
{
    SystemFunc tickSystems[MAX_SYSTEMS];
    SystemFunc stepSystems[MAX_SYSTEMS];
    SystemFunc draw2dSystems[MAX_SYSTEMS];
    SystemFunc draw3dSystems[MAX_SYSTEMS];

    int            activeEntities[MAX_ENTS];
    Active         actives[MAX_ENTS];
    Mask           masks[MAX_ENTS];
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
    CompUiView     uiElements[MAX_ENTS];
    CompMovement   movements[MAX_ENTS];
    CompController controllers[MAX_ENTS];
    CompAbility    abilities[MAX_ENTS];

    bool worldActive;

    int stepCount;
    int tickCount;
    int draw3dCount;
    int draw2dCount;
    int activeCount;
    int playerID;
    u32 systemBits;

    WorldPhysx  *spatial;
    WorldLines  *lines;
    SolEmitters *emitters;
    SolEvents   *events;
} World;

World *World_Create(void);
World *World_Create_Default(void);
void   World_Destroy(World *world);
void   World_System_Add(World *world, WorldSystem system);
int    Sol_World_GetEntCount(World *world);

// Add thing to world
void Sol_World_Line_Add(World *world, vec3s a, vec3s b, vec3s color, vec3s bColor, float ttl);

// Add Entity
int  Sol_Create_Ent(World *world);
void Sol_Destroy_Ent(World *world, int id);

// Add Prefab entity
int Sol_Prefab_Floor(World *world, vec3s pos);
int Sol_Prefab_Pawn(World *world, vec3s pos, SolModelId modelId, float height);
int Sol_Prefab_Button(World *world, vec3s pos, const char *text);
int Sol_Prefab_Ball(World *world, vec3s pos, vec3s vel, CompSphere sphere);
int Sol_Prefab_Boxman(World *world, vec3s pos);

void Sol_Flags_Add(World *world, int id, EntFlags flags);
void Sol_Flags_Remove(World *world, int id, EntFlags flags);

CompXform *Sol_Xform_Add(World *world, int id, vec3s pos);
void       Xform_Snapshot(World *world);
void       Xform_Interpolate(World *world, float alpha);
void       Xform_Teleport(CompXform *xform, vec3s pos);

CompBody  *Sol_Body_Add(World *world, int id, CompBody init);
CompBody  *Sol_Body2_Add(World *world, int id);
CompShape *Sol_Shape_Add(World *world, int id);
void       Physx_Init(World *world);
void       Physx_Step(World *world, double dt, double time);
void       Sol_System_Step_Physx_2d(World *world, double dt, double time);

CompController *Sol_ControllerLocal_Add(World *world, int id);
CompController *Sol_ControllerRemote_Add(World *world, int id);
CompController *Sol_ControllerAi_Add(World *world, int id);
void            Sol_System_Controller_Local_Tick(World *world, double dt, double time);
void            Sol_System_Controller_Ai_Tick(World *world, double dt, double time);

CompMovement *Sol_Movement_Add(World *world, int id, CompMovement init);
void          Sol_System_Movement_2d_Step(World *world, double dt, double time);
void          Sol_System_Movement_3d_Step(World *world, double dt, double time);

CompBuff *Sol_Buff_Add(World *world, int id, CompBuff init);
void      Buff_Step(World *world, double dt, double time);

CompModel *Sol_Model_Add(World *world, int id, CompModel init);
void       Sol_System_Model_Draw(World *world, double dt, double time);
void       Sol_Model_PlayAnim(World *world, int id, SolAnims anim, float blendSpeed);

CompAbility *Sol_Ability_Add(World *world, int id, CompAbility init);
void         Ability_Tick(World *world, double dt, double time);

CompInteract *Sol_Interact_Add(World *world, int id);
void          System_Interact_Tick(World *world, double dt, double time);

void         Sol_System_Camera_Tick(World *world, double dt, double time);
SolRayResult Sol_ScreenRaycast(World *world, float screenX, float screenY, SolRay ray);

void Lines_Init(World *world);
void Sol_System_Line_Tick(World *world, double dt, double time);
void Sol_Draw_Line(SolLine *lines, int count);

CompUiView *Sol_UiView_Add(World *world, int id);
CompSphere *Sol_Sphere_Add(World *world, int id, CompSphere init);
CompTimer  *Sol_Timer_Add(World *world, int id, CompTimer init);
void        Vital_Step(World *world, double dt, double time);
void        Pickup_Step(World *world, double dt, double time);

void Sol_Timer_Tick(World *world, double dt, double time);
void Sol_Sphere_Step(World *world, double dt, double time);

void Sol_Line_Draw(World *world, double dt, double time);
void Sol_UiView_Draw(World *world, double dt, double time);

void Sol_Draw_Rectangle(SolRect rect, vec4s color, float thickness);
void Sol_Cam3d_Tick(World *world, double dt, double time, float alpha);
void Sol_Vital_Draw(World *world, double dt, double time);
void Sol_Crosshair_Draw(World *world, double dt, double time);
void Sol_Sphere_Draw(World *world, double dt, double time);

void Sol_Sphere_ColorAll(World *world, vec4s color);

void Emitter_Init(World *world);
void Emitter_Add(World *world, Emitter e);
void Emitter_Step(World *world, double dt, double time);
void Emitter_Tick(World *world, double dt, double time);
void Emitter_Draw(World *world, double dt, double time);

void Sol_Event_Add(World *world, EventDesc desc);
void Event_Init(World *world);
void Event_Clear(World *world);