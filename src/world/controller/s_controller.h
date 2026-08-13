#pragma once
#include "types.h"

#define LOCAL_AMNT 1

typedef struct CompControllerLocal
{
    int   localIndex;
    int   hoverId, focusId, draggedId;
    vec2s dragOffset;
} CompControllerLocal;

typedef struct CompControllerAi
{
    int targetId;
} CompControllerAi;

typedef struct CompControllerRemote
{
    int remoteId, localId;
} CompControllerRemote;

typedef struct CompController
{
    vec3s      lookdir, wishdir, aimdir, aimpos, aimHitPos;
    SolActions actionState;
    float      yaw, pitch;
    float      zoom;
    u32        aimHitEnt;
    bool       isStrafing;
    bool       pendingShot;
    u64        pendingShotId;
} CompController;

void                  Sol_Controller_Init(World *world);
void                  Sol_Controller_Add(World *world, int id);
CompControllerLocal  *Sol_ControllerLocal_Add(World *world, int id);
CompControllerRemote *Sol_ControllerRemote_Add(World *world, int id);
CompControllerAi     *Sol_ControllerAi_Add(World *world, int id);
vec3s                 Sol_Controller_GetAimPos(World *world, int id);
SolActions            Sol_GetActions(World *world, int id);
vec3s                 Sol_GetWishdir(World *world, int id);
vec3s                 Sol_GetWishdir2(World *world, int id);
vec3s                 Sol_GetLookdir(World *world, int id);
vec3s                 Sol_GetAimpos(World *world, int id);
vec3s                 Sol_Controller_GetAimdir(World *world, int id);
float                 Sol_GetYaw(World *world, int id);
float                 Sol_GetPitch(World *world, int id);
vec3s                 Sol_Controller_GetWishdir(World *world, int id);
vec3s                 Sol_Controller_GetShootPos(World *world, int id, float offset);
SolShoot              Sol_Controller_GetShoot(World *world, int id, float speed);
bool                  Sol_Controller_IsActionState(World *world, int id, SolActions mask);
bool                  Sol_Controller_WantsMove(World *world, int id);