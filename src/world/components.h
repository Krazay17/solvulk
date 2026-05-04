#pragma once
#include "sol/types.h"

typedef enum
{
    HAS_NONE              = 0,
    HAS_XFORM             = (1 << 0),
    HAS_BODY2             = (1 << 1),
    HAS_BODY3             = (1 << 2),
    HAS_TOUCH             = (1 << 3),
    HAS_SHAPE             = (1 << 4),
    HAS_INTERACT          = (1 << 5),
    HAS_MODEL             = (1 << 6),
    HAS_INFO              = (1 << 7),
    HAS_UIVIEW            = (1 << 8),
    HAS_MOVEMENT          = (1 << 9),
    HAS_CONTROLLER        = (1 << 10),
    HAS_CAMERA            = (1 << 13),
    HAS_COMBAT            = (1 << 14),
    HAS_BUFF              = (1 << 15),
    HAS_VITAL             = (1 << 16),
    HAS_SPHERE            = (1 << 17),
    HAS_TIMER             = (1 << 18),
    HAS_PICKUP            = (1 << 19),
    HAS_EVENT             = (1 << 20),
} CompBits;

// XFORM--------------
typedef struct CompXform CompXform;
typedef struct
{
    vec3s   pos, lastPos, drawPos;
    versors quat, lastQuat, drawQuat;
    vec3s   scale, lastScale, drawScale;
} XformDesc;
void  Sol_Xform_Init(World *world);
void  Sol_Xform_Add(World *world, int id, vec3s pos);
void  Xform_Snapshot(World *world);
void  Xform_Interpolate(World *world, float alpha);
void  Sol_Xform_Teleport(World *world, int id, vec3s pos);
vec3s Sol_Xform_GetPos(World *world, int id);
void  Sol_Xform_SetYaw(World *world, int id, float yaw);
void  Sol_Xform_SetPos(World *world, int id, vec3s pos);

// CAMERA----------------
void Sol_Cam3d_Tick(World *world, double dt, double time, float alpha);
void Sol_Crosshair_Draw(World *world, double dt, double time);
void Sol_System_Camera_Tick(World *world, double dt, double time);

// MOVEMENT--------------
typedef struct CompMovement CompMovement;
typedef enum
{
    MOVE_CONFIG_PLAYER,
    MOVE_CONFIG_WIZARD,
    MOVE_CONFIG_COUNT,
} MoveConfigId;
typedef struct
{
    MoveConfigId configId;
} MovementDesc;
void Sol_Movement_Init(World *world);
void Sol_Movement_Add(World *world, int id, MovementDesc desc);
void Sol_System_Movement_2d_Step(World *world, double dt, double time);
void Sol_System_Movement_3d_Step(World *world, double dt, double time);

// PHYSX----------------
typedef struct CompBody CompBody;
typedef struct
{
    vec3s  vel, gravity;
    float  radius, height, length;
    float  mass, restitution;
    Shape3 shape;
    u8     group;
} BodyDesc;
void         Sol_Body_Add(World *world, int id, BodyDesc desc);
void         Sol_Physx_Init(World *world);
void         Sol_Physx_Step(World *world, double dt, double time);
void         Sol_Physx2d_Step(World *world, double dt, double time);
vec3s        Sol_Physx_GetVel(World *world, int id);
void         Sol_Physx_SetVel(World *world, int id, vec3s vel);
SolRayResult Sol_ScreenRaycast(World *world, float screenX, float screenY, SolRay ray);
void         Sol_Physx_SetGrav(World *world, int id, vec3s vel);

// CONTROLLER------------
typedef struct CompController CompController;
typedef enum
{
    CONTROLLER_LOCAL,
    CONTROLLER_REMOTE,
    CONTROLLER_AI,
} ControllerKind;
typedef struct
{
    ControllerKind kind;
} ControllerDesc;
void Sol_Controller_Init(World *world);
void  Sol_Controller_Add(World *world, int id, ControllerDesc desc);
void  Sol_Controller_Tick(World *world, double dt, double time);
vec3s Sol_Controller_GetAimPos(World *world, int id);

// EVENT------------------
void Sol_Event_Add(World *world, EventDesc desc);
void Event_Init(World *world);
void Event_Clear(World *world);

// MODEL------------------
typedef struct CompModel CompModel;
typedef struct
{
    SolModelId id;
    float      yoffset;
} ModelDesc;
void      Sol_Model_Init(World *world);
void      Sol_Model_Add(World *world, int id, ModelDesc desc);
void      Sol_Model_Draw(World *world, double dt, double time);
void      Sol_Model_PlayAnim(World *world, int id, SolAnims anim, float blendSpeed);
SolModel *Sol_Model_GetModel(World *world, int id);

// EMITTER--------------
void Emitter_Init(World *world);
void Emitter_Add(World *world, Emitter e);
void Emitter_Step(World *world, double dt, double time);
void Emitter_Tick(World *world, double dt, double time);
void Emitter_Draw(World *world, double dt, double time);

// LINE-----------------
typedef struct CompLine CompLine;

typedef struct
{
    vec3s a, b;
    float ttl;
    vec4s colorA, colorB;
} LineDesc;
void Sol_Line_Init(World *world);
void Sol_Line_Add(World *world, LineDesc desc);
void Sol_Line_Tick(World *world, double dt, double time);
void Sol_Line_Draw(World *world, double dt, double time);

// SPHERE----------------
typedef struct CompSphere CompSphere;
typedef struct
{
    float radius;
    vec4s color;
} SphereDesc;
void Sol_Sphere_Init(World *world);
void Sol_Sphere_Add(World *world, int id, SphereDesc desc);
void Sol_Sphere_Step(World *world, double dt, double time);
void Sol_Sphere_Draw(World *world, double dt, double time);
void Sol_Sphere_ColorAll(World *world, vec4s color);

// TIMER-----------------
typedef struct CompTimer
{
    float elapsed, duration;
} CompTimer;

// ABILITY---------------
typedef struct CompAbility CompAbility;
typedef struct
{
    u32 hasAbilies;
} AbilityDesc;
void Sol_Ability_Init(World *world);
void Sol_Ability_Add(World *world, int id, AbilityDesc desc);
void Sol_Ability_Step(World *world, double dt, double time);
void Sol_Ability_Tick(World *world, double dt, double time);

// BUFF-----------------
typedef struct CompBuff CompBuff;
typedef enum
{
    BUFF_KNOCKBACK = (1 << 0),
    BUFF_FIRE      = (1 << 1),
    BUFF_COUNT,
} BuffKind;
typedef struct
{
    BuffKind kind;
    float    duration;
} BuffDesc;
void Sol_Buff_Init(World *world);
void Sol_Buff_Add(World *world, int id, BuffDesc desc);
void Sol_Buff_Remove(World *world, int id, BuffKind kind);
void Sol_Buff_Step(World *world, double dt, double time);

// FLAGS----------------
typedef enum
{
    EFLAG_PICKUPABLE = (1 << 0),
    EFLAG_PICKEDUP   = (1 << 1),
} EntFlags;

typedef struct CompFlags
{
    EntFlags flags;
} CompFlags;

// VITAL-----------------
typedef struct CompVital CompVital;
typedef struct
{
    u32   maxHealth, maxEnergy, maxMana;
    u32   health, energy, mana;
    bool  doesRespawn;
    float deathTime, respawnTime;
} VitalDesc;
void Sol_Vital_Init(World *world);
void Sol_Vital_Add(World *world, int id, VitalDesc desc);
void Sol_Vital_Draw(World *world, double dt, double time);
void Sol_Vital_Step(World *world, double dt, double time);

// INTERACT--------------
typedef struct CompInteract CompInteract;

typedef struct
{
    u8       states;
    float    value;
    Callback onClick;
    Callback onHold;
} InteractDesc;
void          Sol_Interact_Init(World *world);
void          Sol_Interact_Add(World *world, int id, InteractDesc desc);
void          System_Interact_Tick(World *world, double dt, double time);
InteractState Sol_Interact_GetState(World *world, int id);

// UI-------------
typedef struct CompUi CompUi;
typedef enum
{
    UI_BUTTON,
    UI_SLIDER,
    UI_COUNT,
} UiKind;
typedef struct
{
    UiKind kind;
} UiDesc;
void Sol_Ui_Add(World *world, int id, UiDesc desc);
void Sol_Ui_Draw(World *world, double dt, double time);

// PICKUP---------------
void Sol_Pickup_Init(World *world);
void Sol_Pickup_Step(World *world, double dt, double time);
