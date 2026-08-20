#pragma once
#include "sol/types.h"

typedef enum ControllerKind
{
    CONTROLLERKIND_NONE,
    CONTROLLERKIND_USER,
    CONTROLLERKIND_PLAYER,
    CONTROLLERKIND_SPECTATE,
    CONTROLLERKIND_REMOTE,
    CONTROLLERKIND_WIZARD,
    CONTROLLERKIND_ZORGON,
} ControllerKind;

typedef struct CompController
{
    u8         kind;
    SolActions actionState;
    int        aimHitEnt;
    float      yaw, pitch;

    vec3s wishdir, wishdirY, aimdir, aimpos, lookdir, knockDur;
    vec2s wishdir2d, aimpos2d;

    bool isStrafing;
} CompController;

void Sol_Controller_Init(World *world);

CompController *Sol_Controller_Add(World *world, int id);
void            Sol_Controller_Remove(World *world, int id);
CompController *Sol_Controller_Get(World *world, int id);

void Sol_Controller_SetParallaxAim(World *world, int id, vec3s lookpos, vec3s lookdir, float range, float hitdepth);
// SolActions Sol_Controller_Get(world, id)->actionState(World *world, int id);
// vec3s      Sol_GetWishdir(World *world, int id);
// vec3s      Sol_GetLookdir(World *world, int id);
// vec3s      Sol_GetAimpos(World *world, int id);
// float      Sol_GetYaw(World *world, int id);
// float      Sol_GetPitch(World *world, int id);
// vec3s      Sol_Controller_GetWishdir(World *world, int id);
// bool       Sol_Controller_IsActionState(World *world, int id, SolActions mask);
// bool       Sol_Controller_WantsMove(World *world, int id);

// deprecated
// void Sol_Controller_Add(World *world, int id, ControllerKind kind);
// vec3s    Sol_Controller_GetAimPos(World *world, int id);
vec3s    Sol_Controller_GetShootPos(World *world, int id, float offset);
SolShoot Sol_Controller_GetShoot(World *world, int id, float offset, float speed);