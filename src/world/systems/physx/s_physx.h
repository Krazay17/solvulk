#pragma once
#include "base.h"
#include "spatial_hashmap.h"
#include "spatial_grid.h"

#define DYNAMIC_CELL_SIZE 2.0f
#define STATIC_CELL_SIZE 4.0f

#define MAX_CONTACTS (1 << 16)
#define TERMINAL_VELOCITY -100.0f

#define MAKE_TRI_VALUE(entID, triIdx) ((((u32)(entID) & 0xFFFF) << 16) | ((u32)(triIdx) & 0xFFFF))
#define GET_TRI_ENTITY(val) (((val) >> 16) & 0xFFFF)
#define GET_TRI_INDEX(val) ((val) & 0xFFFF)

#define WORLD_TRI_INIT 0xfff
#define MAX_THREAD_CONTACTS (1<<12)
#define SOLVER_ITERATIONS 1

typedef struct ScXform ScXform;
typedef struct ScBody3 ScBody3;
typedef struct SolTri  SolTri;

typedef struct
{
    SolContact contacts[MAX_THREAD_CONTACTS];
    u32        count;
} ThreadContactBuffer;

typedef struct
{
    int   id;
    int   shape;
    vec3s pos, dims, vel;
    mat3s rot;
} EntProxy;

typedef struct
{
    SpatialGrid  spatial;
    EntProxy    *proxies;
    SpatialAABB *aabb_scratch;
} DynamicGroup;

typedef struct
{
    SpatialGrid spatial;
    SolTri     *tris;
} StaticGroup;

typedef struct
{
    DynamicGroup dynamic_group;
    StaticGroup  static_group;
    SolContact  *contacts;
} SysPhysx;

void Build_Tables(World *world, SysPhysx *sys, float fdt);
void Resolve_Contact(ScBody3 *bodyA, ScXform *xformA, ScBody3 *bodyB, ScXform *xformB, SolContact *contact);

void Collisions_Static_Stage_Local(World *world, int idA, Shape3 shape, vec3s min, vec3s max, StaticGroup *group,
                                   ThreadContactBuffer *contacts);
void Collisions_Dynamic_Bodies_Local(World *world, int idA, Shape3 shape, vec3s min, vec3s max, DynamicGroup *group,
                                     ThreadContactBuffer *contacts);

void Collisions_Static_Stage(World *world, int idA, SysPhysx *sys, ScXform *xform, ScBody3 *body, float fdt);
void Collisions_Dynamic_Bodies(World *world, int idA, SysPhysx *sys, ScXform *xform, ScBody3 *body);
void Collisions_Dynamic_Tris(World *world, int idA, SysPhysx *sys, ScXform *xform, ScBody3 *body);

bool Collide_Sphere_Sphere(World *world, int idA, int idB, SolContact *hit);
bool Collide_Capsule_Capsule(World *world, int idA, int idB, SolContact *hit);
bool Collide_Sphere_Capsule(World *world, int idA, int idB, SolContact *hit);
bool Collide_Capsule_Sphere(World *world, int idA, int idB, SolContact *hit);
bool Collide_Box_Box(World *world, int idA, int idB, SolContact *hit);
bool Collide_Box_Sphere(World *world, int idA, int idB, SolContact *hit);
bool Collide_Sphere_Box(World *world, int idA, int idB, SolContact *hit);

bool Collide_Sphere_Tri(World *world, int idA, int idB, const SolTri *tri, SolContact *hit);
bool Collide_Capsule_Tri(World *world, int idA, int idB, const SolTri *tri, SolContact *hit);

bool Is_Already_Hit(const SolRayResult *results, int hitCount, int entId);
bool Ray_Intersect_Tri(vec3s origin, vec3s dir, float maxDist, const SolTri *tri, float *outT, vec3s *outNorm);
bool Ray_Intersect_Sphere(vec3s origin, vec3s dir, float maxDist, vec3s center, float radius, float *outT,
                          vec3s *outNorm);
