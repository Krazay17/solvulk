#pragma once
#include "sol/types.h"

typedef struct
{
    vec3s pos, vel;
} SolShoot;

typedef enum
{
    CONTROLLER_LOCAL = 1,
    CONTROLLER_REMOTE,
} ControllerKind;

typedef struct CompController
{
    vec3s          lookdir, wishdir, aimdir, aimpos, aimHitPos;
    SolActions     actionState;
    float          yaw, pitch;
    float          zoom;
    u32            aimHitEnt;
    ControllerKind kind;
    bool           isStrafing;
} CompController;

void Sol_Controller_Init(World *world);

void Sol_Controller_Add(World *world, int id, ControllerKind kind);

vec3s      Sol_Controller_GetAimPos(World *world, int id);
SolActions Sol_GetActions(World *world, int id);
vec3s      Sol_GetWishdir(World *world, int id);
vec3s      Sol_GetWishdir2(World *world, int id);
vec3s      Sol_GetLookdir(World *world, int id);
vec3s      Sol_GetAimpos(World *world, int id);
vec3s      Sol_Controller_GetAimdir(World *world, int id);
float      Sol_GetYaw(World *world, int id);
float      Sol_GetPitch(World *world, int id);
vec3s      Sol_Controller_GetWishdir(World *world, int id);
SolActions Sol_Controller_GetActionState(World *world, int id);
vec3s      Sol_Controller_GetShootPos(World *world, int id, float offset);
SolShoot   Sol_Controller_GetShoot(World *world, int id, float speed);
bool       Sol_Controller_IsActionState(World *world, int id, SolActions mask);
