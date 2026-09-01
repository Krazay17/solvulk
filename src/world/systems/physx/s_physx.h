#pragma once
#include "base.h"
#include "spatial_hashmap.h"
#include "spatial_grid.h"
#include "sol_array.h"

#define SPATIAL_SIZE (1 << 20)
#define SPATIAL_CAP (1 << 24)
#define SPATIAL_CELL_SIZE 1.0f

#define SPATIAL_TRI_SIZE (1 << 12)
#define SPATIAL_TRI_CAP (1 << 20)
#define SPATIAL_TRI_CELL_SIZE 4.0f

#define SPATIAL_STATIC_TRI_CELL_SIZE 1.0f

#define MAX_CONTACTS (1 << 16)
#define TERMINAL_VELOCITY -100.0f

#define MAKE_TRI_VALUE(entID, triIdx) ((((u32)(entID) & 0xFFFF) << 16) | ((u32)(triIdx) & 0xFFFF))
#define GET_TRI_ENTITY(val) (((val) >> 16) & 0xFFFF)
#define GET_TRI_INDEX(val) ((val) & 0xFFFF)

#define WORLD_TRI_INIT 0xfff
#define MAX_THREAD_CONTACTS 1024

typedef struct SolXform SolXform;
typedef struct SolBody3 SolBody3;
typedef struct SolTri   SolTri;

typedef struct
{
    bool  hit;
    float t;      // Normalized time of impact [0.0, 1.0]
    vec3s point;  // Contact point in world space
    vec3s normal; // Triangle / surface normal at hit point
} SolSweptHit;

typedef struct
{
    SolContact contacts[MAX_THREAD_CONTACTS];
    u32        count;
} ThreadContactBuffer;

typedef struct
{
    SpatialTable dynamic_table;
    SpatialTable dynamic_tri_table;
    SpatialGrid  static_tri_grid;
    SolContact  *contacts;
    SolTri      *worldTris;
} SysPhysx;
typedef struct
{
    float invMass;
    float restitution;
    vec3s vel;
} BodyData;

void Build_Tables(World *world, SysPhysx *sys, float fdt);
void Resolve_Contact(SolBody3 *bodyA, SolXform *xformA, SolBody3 *bodyB, SolXform *xformB, SolContact *contact);

void Collisions_Static_Stage(World *world, int idA, SysPhysx *sys, SolXform *xform, SolBody3 *body, float fdt);
void Collisions_Static_Stage_Local(World *world, int idA, SysPhysx *sys, SolXform *xform, SolBody3 *body, float fdt,
                                   ThreadContactBuffer *contacts);

void Collisions_Dynamic_Bodies(World *world, int idA, SysPhysx *sys, SolXform *xform, SolBody3 *body);
void Collisions_Dynamic_Bodies_Local(World *world, int idA, SysPhysx *sys, SolXform *xform, SolBody3 *body,
                                     ThreadContactBuffer *contacts);
void Collisions_Dynamic_Tris(World *world, int idA, SysPhysx *sys, SolXform *xform, SolBody3 *body);

bool Collide_Sphere_Sphere(World *world, int idA, int idB, SolContact *hit);
bool Collide_Capsule_Capsule(World *world, int idA, int idB, SolContact *hit);
bool Collide_Sphere_Capsule(World *world, int idA, int idB, SolContact *hit);
bool Collide_Capsule_Sphere(World *world, int idA, int idB, SolContact *hit);
bool Collide_Box_Box(World *world, int idA, int idB, SolContact *hit);
bool Collide_Box_Sphere(World *world, int idA, int idB, SolContact *hit);
bool Collide_Sphere_Box(World *world, int idA, int idB, SolContact *hit);

bool Collide_Sphere_Tri(World *world, int idA, int idB, const SolTri *tri, SolContact *hit);
bool Collide_Capsule_Tri(World *world, int idA, int idB, SolContact *hit);
