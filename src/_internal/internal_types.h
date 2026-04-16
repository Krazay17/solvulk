#pragma once
#include <cglm/types-struct.h>

typedef struct
{
    vec3s pos, normal, vel;
    int id;
    bool didCollide;
} SolCollision;

typedef struct
{
    vec3s a, b, c;
    vec3s normal;
} CollisionTri;

typedef struct
{
    u32 *head;
    u32 *value;
    u32 *next;
    u32 capacity;
    u32 count;
} SpatialTable;

typedef struct WorldSpatial
{
    SpatialTable staticWorld;
    SpatialTable dynamicUnits;

    CollisionTri *tris;
    u32 triCount;
} WorldSpatial;

typedef struct
{
    int ix, iy, iz;
    u32 neighborHashes[27];
} SpatialCell;