#pragma once
#include "types.h"

typedef enum ControllerKind
{
    CONTROLLERKIND_NONE,
    CONTROLLERKIND_PLAYER,
    CONTROLLERKIND_REMOTE,
    CONTROLLERKIND_WIZARD,
    CONTROLLERKIND_ZORGON,
} ControllerKind;

typedef struct CompController
{
    SolActions actionState;
    int        aimHitEnt;
    float      yaw, pitch;

    vec3s wishdir, aimdir, aimpos, lookdir, knockDur;
    vec2s wishdir2d, aimpos2d;

    bool isStrafing;
} CompController;

void       Sol_Controller_Init(World *world);
void       Sol_Controller_Add(World *world, int id, ControllerKind kind);
vec3s      Sol_Controller_GetAimPos(World *world, int id);
SolActions Sol_GetActions(World *world, int id);
vec3s      Sol_GetWishdir(World *world, int id);
vec3s      Sol_GetLookdir(World *world, int id);
vec3s      Sol_GetAimpos(World *world, int id);
vec3s      Sol_Controller_GetAimdir(World *world, int id);
float      Sol_GetYaw(World *world, int id);
float      Sol_GetPitch(World *world, int id);
vec3s      Sol_Controller_GetWishdir(World *world, int id);
vec3s      Sol_Controller_GetShootPos(World *world, int id, float offset);
SolShoot   Sol_Controller_GetShoot(World *world, int id, float speed);
bool       Sol_Controller_IsActionState(World *world, int id, SolActions mask);
bool       Sol_Controller_WantsMove(World *world, int id);
void Sol_Controller_SetParallaxAim(World *world, int id, vec3s lookpos, vec3s lookdir, float range, float hitdepth);