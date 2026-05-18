#pragma once
#include "sol/types.h"

typedef struct AiController
{
    vec3s    lookdir, wishdir;
    AiAction actionState;
} AiController;

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
void       Sol_Controller_Init(World *world);
void       Sol_Controller_Add(World *world, int id, ControllerDesc desc);
void       Sol_Controller_Tick(World *world, double dt, double time);
vec3s      Sol_Controller_GetAimPos(World *world, int id);
SolActions Sol_GetActions(World *world, int id);
vec3s      Sol_GetWishdir(World *world, int id);
vec2s      Sol_GetWishdir2(World *world, int id);
vec3s      Sol_GetLookdir(World *world, int id);
vec3s      Sol_GetAimpos(World *world, int id);
vec3s      Sol_GetAimdir(World *world, int id);
float      Sol_GetYaw(World *world, int id);
float      Sol_GetPitch(World *world, int id);
vec3s      Sol_Controller_GetWishdir(World *world, int id);
SolActions Sol_Controller_GetActionState(World *world, int id);
vec3s Sol_Controller_GetShootpos(World *world, int id, float offset);