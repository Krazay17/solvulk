#pragma once
#include "base.h"

typedef struct SolBody3 SolBody3;
typedef struct SolXform SolXform;
typedef struct SolTri SolTri;

typedef struct SolContact
{
    u32   id, idB;
    vec3s pos, normal;
    float penetration;
} SolContact;

bool Collide_Sphere_Sphere(World *world, int idA, int idB, SolContact *hit);
bool Collide_Capsule_Capsule(World *world, int idA, int idB, SolContact *hit);
bool Collide_Sphere_Capsule(World *world, int idA, int idB, SolContact *hit);
bool Collide_Capsule_Sphere(World *world, int idA, int idB, SolContact *hit);
bool Collide_Box_Box(World *world, int idA, int idB, SolContact *hit);
bool Collide_Box_Sphere(World *world, int idA, int idB, SolContact *hit);
bool Collide_Sphere_Box(World *world, int idA, int idB, SolContact *hit);

bool Collide_Sphere_Tri(World *world, int idA, int idB, const SolTri *tri, SolContact *hit);
bool Collide_Capsule_Model(World *world, int idA, int idB, SolContact *hit);
