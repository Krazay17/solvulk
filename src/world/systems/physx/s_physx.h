#pragma once
#include "base.h"
#include "spatial_hashmap.h"
#include "spatial_grid.h"

#define SPATIAL_SIZE (1 << 12)
#define SPATIAL_CAP (1 << 18)
#define SPATIAL_CELL_SIZE 2.0f

#define SPATIAL_TRI_SIZE (1 << 12)
#define SPATIAL_TRI_CAP (1 << 20)
#define SPATIAL_TRI_CELL_SIZE 4.0f

#define SPATIAL_STATIC_TRI_CELL_SIZE 2.0f

#define MAX_CONTACTS (1 << 16)
#define TERMINAL_VELOCITY -100.0f

#define MAKE_TRI_VALUE(entID, triIdx) ((((u32)(entID) & 0xFFFF) << 16) | ((u32)(triIdx) & 0xFFFF))
#define GET_TRI_ENTITY(val) (((val) >> 16) & 0xFFFF)
#define GET_TRI_INDEX(val) ((val) & 0xFFFF)

#define MAX_WORLD_TRIS 0xfffff

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

typedef struct SolContact
{
    u32   id, idB;
    vec3s pos, normal;
    float penetration;
} SolContact;

typedef struct
{
    SolContact contact[MAX_CONTACTS];
    int        contact_cnt;
} Body3Contacts;

typedef struct
{
    SpatialTable  dynamic_table;
    SpatialTable  dynamic_tri_table;
    SpatialTable  static_tri_table;
    SpatialGrid   static_tri_grid;
    SolTri        worldTris[MAX_WORLD_TRIS];
    Body3Contacts contacts;
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

void Collisions_Dynamic_Bodies(World *world, int idA, SysPhysx *sys, SolXform *xform, SolBody3 *body);
void Collisions_Dynamic_Tris(World *world, int idA, SysPhysx *sys, SolXform *xform, SolBody3 *body);
void Collisions_Static_Tris(World *world, int idA, SysPhysx *sys, SolXform *xform, SolBody3 *body3);
bool Collisions_Swept_Static_Tris(World *world, int idA, SysPhysx *sys, SolXform *xform, SolBody3 *body, float dt,
                                  SolSweptHit *earliestHit);

bool Collide_Sphere_Sphere(World *world, int idA, int idB, SolContact *hit);
bool Collide_Capsule_Capsule(World *world, int idA, int idB, SolContact *hit);
bool Collide_Sphere_Capsule(World *world, int idA, int idB, SolContact *hit);
bool Collide_Capsule_Sphere(World *world, int idA, int idB, SolContact *hit);
bool Collide_Box_Box(World *world, int idA, int idB, SolContact *hit);
bool Collide_Box_Sphere(World *world, int idA, int idB, SolContact *hit);
bool Collide_Sphere_Box(World *world, int idA, int idB, SolContact *hit);

bool Collide_Sphere_Tri(World *world, int idA, int idB, const SolTri *tri, SolContact *hit);
bool Collide_Capsule_Tri(World *world, int idA, int idB, SolContact *hit);
