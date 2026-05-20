#pragma once
#include "sol/types.h"

typedef struct
{
    u32 enemyKind;
} AiControllerDesc;

typedef enum
{
    CONTROLLER_LOCAL,
    CONTROLLER_REMOTE,
} ControllerKind;
typedef struct
{
    ControllerKind kind;
} ControllerDesc;

void Sol_Controller_Init(World *world);
void Sol_AiController_Init(World *world);

void Sol_Controller_Add(World *world, int id, ControllerDesc desc);
void Sol_AiController_Add(World *world, int id, AiControllerDesc desc);
void Sol_AiController_Clear(World *world, int id);

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
vec3s      Sol_Controller_GetShootpos(World *world, int id, float offset);

void Sol_AiController_SetLastHit(World *world, int id, int source, u32 damage);
void Sol_AiController_TargetDied(World *world, int id, int target);