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
    HAS_CONTROLLER_AI     = (1 << 11),
    HAS_CONTROLLER_REMOTE = (1 << 12),
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
typedef struct
{
    vec3s   pos, lastPos, drawPos;
    versors quat, lastQuat, drawQuat;
    vec3s   scale, lastScale, drawScale;
} XformDesc;
void   Sol_Xform_Add(World *world, int id, vec3s pos);
void   Xform_Snapshot(World *world);
void   Xform_Interpolate(World *world, float alpha);
void   Xform_Teleport(CompXform *xform, vec3s pos);
vec3s *Sol_Xform_GetPos(World *world, int id);
void   Sol_Xform_SetYaw(World *world, int id, float yaw);
void   Sol_Xform_SetPos(World *world, int id, vec3s pos);

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
void Sol_Movement_Add(World *world, int id, MovementDesc desc);
void Sol_System_Movement_2d_Step(World *world, double dt, double time);
void Sol_System_Movement_3d_Step(World *world, double dt, double time);

// PHYSX----------------
typedef struct
{
    vec3s  vel, gravity;
    float  radius, height, length;
    float  mass, restitution;
    Shape3 shape;
    u8     group;
} BodyDesc;
void  Sol_Body_Add(World *world, int id, BodyDesc desc);
void  Sol_Physx_Init(World *world);
void  Sol_Physx_Step(World *world, double dt, double time);
void  Sol_Physx2d_Step(World *world, double dt, double time);
vec3s Sol_Physx_GetVel(World *world, int id);
void  Sol_Physx_SetVel(World *world, int id, vec3s vel);

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
void Sol_Controller_Add(World *world, int id, ControllerDesc desc);
void Sol_Controller_Tick(World *world, double dt, double time);

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

// SPHERE----------------
typedef struct CompSphere CompSphere;
typedef struct
{
    float radius;
    vec4s color;
} SphereDesc;
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
typedef struct CompVital
{
    u32   maxHealth, maxEnergy, maxMana;
    u32   health, energy, mana;
    bool  doesRespawn;
    float deathTime, respawnTime;
} CompVital;

// INTERACT--------------
typedef enum InteractState
{
    INTERACT_HOVERED  = (1 << 0),
    INTERACT_PRESSED  = (1 << 1),
    INTERACT_CLICKED  = (1 << 2),
    INTERACT_TOGGLED  = (1 << 3),
    INTERACT_ISTOGGLE = (1 << 4),
} InteractState;
typedef struct
{
    u8       states;
    float    value;
    Callback onClick;
    Callback onHold;
} InteractDesc;
void Sol_Interact_Add(World *world, int id, InteractDesc desc);
void System_Interact_Tick(World *world, double dt, double time);

// UI-------------
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
