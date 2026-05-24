#pragma once
#include "sol/sol.h"

#include "estate.h"
#include "movement.h"

typedef struct
{
    double lastEntered, lastExited;
    float  elapsed;
    vec3s  enterVel, surfaceNormal, dir;
} MoveStateData;

typedef struct CompMovement
{
    vec3s         updir, dashdir;
    float         baseHeight, targetHeight;
    float         stateTimer;
    float         speedMod;
    MoveState     moveState;
    MoveConfigId  configId;
    bool          wantsJump, jumpPressedLastFrame;
    MoveStateData stateData[MOVE_STATE_COUNT];
} CompMovement;

typedef struct
{
    float speed, accell, friction, gravity;
} MoveStateForce;

extern const MoveStateForce MOVE_STATE_FORCES[MOVE_CONFIG_COUNT][MOVE_STATE_COUNT];
extern const StateFunc      MOVE_STATE_FUNCS[MOVE_STATE_COUNT];

void Sol_Movement_Prestep(World *world, double dt, double time);

bool  Sol_Movement_SetState(World *world, int id, MoveState state);
void  CrouchHeight(World *world, int id, float fdt);
vec3s GroundSlope(World *world, int id);
vec3s ProjectOntoGround(World *world, int id, vec3s wishdir);

void Sol_Movement_Idle_Update(World *world, int id, float dt);
void Sol_Movement_Idle_Enter(World *world, int id);
void Sol_Movement_Idle_Exit(World *world, int id);
bool Sol_Movement_Idle_CanExit(World *world, int id, u32 next);
bool Sol_Movement_Idle_CanEnter(World *world, int id, u32 last, u32 next);

void Sol_Movement_Walk_Update(World *world, int id, float dt);
void Sol_Movement_Walk_Enter(World *world, int id);
void Sol_Movement_Walk_Exit(World *world, int id);
bool Sol_Movement_Walk_CanExit(World *world, int id, u32 next);
bool Sol_Movement_Walk_CanEnter(World *world, int id, u32 last, u32 next);

void Sol_Movement_Jump_Update(World *world, int id, float dt);
void Sol_Movement_Jump_Enter(World *world, int id);
void Sol_Movement_Jump_Exit(World *world, int id);
bool Sol_Movement_Jump_CanExit(World *world, int id, u32 next);
bool Sol_Movement_Jump_CanEnter(World *world, int id, u32 last, u32 next);

void Sol_Movement_Fall_Update(World *world, int id, float dt);
void Sol_Movement_Fall_Enter(World *world, int id);
void Sol_Movement_Fall_Exit(World *world, int id);
bool Sol_Movement_Fall_CanExit(World *world, int id, u32 next);
bool Sol_Movement_Fall_CanEnter(World *world, int id, u32 last, u32 next);

void Sol_Movement_Fly_Update(World *world, int id, float dt);
void Sol_Movement_Fly_Enter(World *world, int id);
void Sol_Movement_Fly_Exit(World *world, int id);
bool Sol_Movement_Fly_CanExit(World *world, int id, u32 next);
bool Sol_Movement_Fly_CanEnter(World *world, int id, u32 last, u32 next);

void Crouch_State_Update(World *world, int id, float dt);
void Crouch_State_Enter(World *world, int id);
void Crouch_State_Exit(World *world, int id);
bool Crouch_State_CanExit(World *world, int id, u32 next);
bool Crouch_State_CanEnter(World *world, int id, u32 last, u32 next);

void Slide_State_Update(World *world, int id, float dt);
void Slide_State_Enter(World *world, int id);
void Slide_State_Exit(World *world, int id);
bool Slide_State_CanExit(World *world, int id, u32 next);
bool Slide_State_CanEnter(World *world, int id, u32 last, u32 next);

void Wallrun_State_Update(World *world, int id, float dt);
void Wallrun_State_Enter(World *world, int id);
void Wallrun_State_Exit(World *world, int id);
bool Wallrun_State_CanExit(World *world, int id, u32 next);
bool Wallrun_State_CanEnter(World *world, int id, u32 last, u32 next);

void Walljump_State_Update(World *world, int id, float dt);
void Walljump_State_Enter(World *world, int id);
void Walljump_State_Exit(World *world, int id);
bool Walljump_State_CanExit(World *world, int id, u32 next);
bool Walljump_State_CanEnter(World *world, int id, u32 last, u32 next);
