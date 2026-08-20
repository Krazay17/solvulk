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
CompController *Sol_Controller_Get(World *world, int id);
void            Sol_Controller_Remove(World *world, int id);

void     Sol_Controller_SetParallaxAim(World *world, int id, vec3s lookpos, vec3s lookdir, float range, float hitdepth);
vec3s    Sol_Controller_GetShootPos(World *world, int id, float offset);
SolShoot Sol_Controller_GetShoot(World *world, int id, float offset, float speed);