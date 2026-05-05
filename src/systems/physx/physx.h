#pragma once
#include "sol/types.h"
#include "world/world.h"

#define SPATIAL_NULL 0xFFFFFFFF

#define SPATIAL_DYNAMIC_CELL_SIZE 3.0f
#define SPATIAL_DYNAMIC_SIZE (1 << 18)
#define SPATIAL_DYNAMIC_ENTRIES 0x2FFFF

#define SPATIAL_STATIC_CELL_SIZE 1.5f
#define SPATIAL_STATIC_SIZE (1 << 21)
#define SPATIAL_STATIC_ENTRIES 0xFFFFFFF

typedef struct CompBody
{
    vec3s  vel, impulse, force, groundNormal, dims;
    vec3s  gravity;
    float  grounded, airtime;
    float  mass, invMass, restitution;
    Shape3 shape;
    u8     group;
} CompBody;

typedef struct CompXform CompXform;
typedef struct SolContact SolContact;

typedef bool (*ShapeTriTest)(CompBody *body, CompXform *xform, SolTri *tri, SolContact *hit);
typedef bool (*ShapePairTest)(CompBody *aa, CompXform *ab, CompBody *ba, CompXform *bb, SolContact *hit);

typedef float (*RaycastTest)(SolRay ray, CompXform *xform, CompBody *body, vec3s *normal);

struct SolContact
{
    vec3s point, normal;
    float penetration;
    bool  didCollide;
};

typedef struct GridWalker
{
    // Walk state
    int   ix, iy, iz;
    int   stepX, stepY, stepZ;
    float tMaxX, tMaxY, tMaxZ;
    float tDeltaX, tDeltaY, tDeltaZ;
    float tEntry; // t at entry of the cell we're about to return
    float maxDist;

    ivec3s dims;
    bool   bounded; // if false, no dims check (infinite hash grid)
} GridWalker;

typedef struct GridCell
{
    int   ix, iy, iz;
    float tEntry, tExit;
} GridCell;

typedef struct SpatialCell
{
    int ix, iy, iz;
    u32 neighborHashes[27];
} SpatialCell;

typedef struct SpatialTable
{
    u32  *head;
    u32  *value;
    u32  *next;
    u32   capacity;
    u32   size;
    u32   count;
    float cellSize;
} SpatialTable;

typedef struct SpatialGrid
{
    u32   *offsets;
    u32   *values;
    u32    built_count;
    float  cellSize;
    vec3s  min;
    vec3s  max;
    ivec3s dims;
} SpatialGrid;

typedef struct PhysxEnts
{
    u32 id;
    u32 triIndexStart;
    u32 triIndexCount;
} PhysxEnts;

typedef struct PhysxGroup
{
    SpatialTable table;
    SpatialGrid  grid;

    SolTri *tris;
    u32     triCount;
    u32     capacity;

    PhysxEnts ents[MAX_ENTS];
    u32       entCount;
} PhysxGroup;

typedef struct WorldPhysx
{
    PhysxGroup staticGroup;
    PhysxGroup dynamicGroup;
} WorldPhysx;

typedef struct
{
    u8    substeps;
    float sub_dt;
} SubstepData;

void Spatial_Add(World *world, int id, CompBody *body);
void Spatial_Add_Model(PhysxGroup *triGroup, int id, SolModel *model, CompXform *xform, bool hash);
void Physx_Grid_Static_Build(PhysxGroup *group, vec3s min, vec3s max, float cell_size);
void Physx_Grid_Static_Rebuild(PhysxGroup *group);
void Transform_Tris_LocalToWorld(SolTri *group, int id, int offset, SolModel *model, CompXform *xform);
void Fill_Dynamic_Table(World *world, int count, int *ents);

// SolContact Collisions_Static_Hashed(PhysxGroup *group, CompBody *body, CompXform *xform, ResolveShapeTri resolver);
// SolContact Collisions_Dynamic_Grid(World *world, int id, CompBody *body, CompXform *xform);

void SpatialTable_Init(SpatialTable *table, u32 buckets, u32 capacity, float cellSize);
void SpatialTable_Clear(SpatialTable *table);
void SpatialTable_Free(SpatialTable *table);
void SpatialTable_Insert(SpatialTable *table, u32 hash, u32 value);
void SpatialTable_Compact(SpatialTable *table);

// Per-entity substep count based on speed
SubstepData Substep_Get(CompBody *body, float fdt);
SpatialCell Spatial_Cell_Get(vec3s pos, float cell_size);
void        Spatial_Hash_Tris(PhysxGroup *group);

// SolRayResult Raycast_Dynamic_Grid_Tri(PhysxGroup *group, SolRay ray);
// SolRayResult Raycast_Dynamic_Table_Tri(PhysxGroup *group, SolRay ray);
// SolRayResult Raycast_Static_Table_Tri(PhysxGroup *group, SolRay ray);
void         Grid_Walker_Init(GridWalker *w, SolRay ray, SpatialGrid *grid);
void         Grid_Walker_Init_Infinite(GridWalker *w, SolRay ray, float cellSize);
bool         Grid_Walker_Next(GridWalker *w, GridCell *out);
SolRayResult Raycast_Static_Grid_Walk(World *world, SolRay ray);

SolRayResult Raycast_Static_Grid_Tri(PhysxGroup *group, SolRay ray);
SolRayResult Raycast_Dynamic_Table_Walk(World *world, SolRay ray);
float        Ray_Sphere_Test(SolRay ray, CompXform *xform, CompBody *body, vec3s *outNormal);
float        Ray_Capsule_Test(SolRay ray, CompXform *xform, CompBody *body, vec3s *outNormal);
float        Ray_Tri_Test(vec3s origin, vec3s dir, SolTri *tri, vec3s *outNormal);
void         Ground_Trace(World *world, int count);

void Collisions_Static_Grid(PhysxGroup *group, CompBody *body, CompXform *xform, SolContact *hit);
void Collisions_Dynamic_Hashed(World *world, int id, CompBody *body, CompXform *xform);

bool Collide_Y(CompXform *xform, CompBody *body, SolContact *hit);
bool Collide_Sphere_Tri(CompBody *body, CompXform *xform, SolTri *tri, SolContact *col);
bool Collide_Capsule_Tri(CompBody *body, CompXform *xform, SolTri *tri, SolContact *col);
bool Collide_Box_Tri(CompBody *body, CompXform *xform, SolTri *tri, SolContact *col);
void Resolve_Contact(CompBody *body, CompXform *xform, SolContact *hit);

bool Collide_Sphere_Sphere(CompBody *aBody, CompXform *aXform, CompBody *bBody, CompXform *bXform, SolContact *hit);
bool Collide_Sphere_Box(CompBody *aBody, CompXform *aXform, CompBody *bBody, CompXform *bXform, SolContact *hit);
void Resolve_Dynamic_Pair(CompBody *aBody, CompXform *aXform, CompBody *bBody, CompXform *bXform, SolContact *hit);

static inline u32 hash_coords(int x, int y, int z)
{
    return ((unsigned int)x * 73856093) ^ ((unsigned int)y * 19349663) ^ ((unsigned int)z * 83492791);
}

static inline u32 cell_index(SpatialGrid *grid, vec3s pos)
{
    int x = (int)floorf((pos.x - grid->min.x) / grid->cellSize);
    int y = (int)floorf((pos.y - grid->min.y) / grid->cellSize);
    int z = (int)floorf((pos.z - grid->min.z) / grid->cellSize);

    x = x < 0 ? 0 : (x >= grid->dims.x ? grid->dims.x - 1 : x);
    y = y < 0 ? 0 : (y >= grid->dims.y ? grid->dims.y - 1 : y);
    z = z < 0 ? 0 : (z >= grid->dims.z ? grid->dims.z - 1 : z);

    return x + y * grid->dims.x + z * grid->dims.x * grid->dims.y;
}
