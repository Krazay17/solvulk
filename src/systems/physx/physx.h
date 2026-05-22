#pragma once
#include "sol/types.h"

typedef struct SolRay
{
    vec3s pos, dir;
    float dist;
    float min;
    u8    mask;
    u32   ignoreEnt;
} SolRay;

typedef struct SolRayResult
{
    bool  hit;
    vec3s pos, norm;
    float dist;
    u32   triIndex;
    u32   entId;
} SolRayResult;

typedef struct
{
    vec3s  vel, gravity;
    float  radius, height, length;
    float  mass, restitution;
    Shape3 shape;
    u8     group;
    bool   is2d, ignoreFriendly;
} BodyDesc;

void         Sol_Body_Add(World *world, int id, BodyDesc desc);
void         Sol_Physx_Init(World *world);
void         Sol_Physx_Step(World *world, double dt, double time);
void         Sol_Physx2d_Step(World *world, double dt, double time);
vec3s        Sol_Physx_GetVel(World *world, int id);
vec3s        Sol_Physx_GetGround(World *world, int id);
bool         Sol_Physx_GetGrounded(World *world, int id);
SolRayResult Sol_ScreenRaycast(World *world, int screenX, int screenY, SolRay ray);
void         Sol_Physx_SetGrav(World *world, int id, vec3s vel);
vec3s        Sol_Physx_GetDims(World *world, int id);
void         Sol_Physx_SetVel(World *world, int id, vec3s vel);
void         Sol_Physx_SetVelX(World *world, int id, float x);
void         Sol_Physx_SetVelY(World *world, int id, float y);
void         Sol_Physx_SetVelZ(World *world, int id, float z);
SolRayResult Sol_Raycast(World *world, SolRay ray);
SolRayResult Sol_RaycastD(World *world, SolRay ray, float debugDuration);
vec3s        Sol_Physx_GetHeadPos(World *world, int id);
float        Sol_Physx_GetSpeed(World *world, int id);
float        Sol_Physx_GetLatSpeed(World *world, int id);
int          Sol_SphereCast(World *world, SolRay ray, float radius, SolRayResult *results, int maxResults);
void         Sol_Physx_SetVellat(World *world, int id, vec3s vel);
void         Sol_Physx_AddVel(World *world, int id, vec3s addvel);
void         Sol_Physx_Impulse(World *world, int id, vec3s impulse);