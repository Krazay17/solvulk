#pragma once
#include <cglm/types-struct.h>

#define MAX_DEVICE_QUERY 8
#define MAX_QUEUE_FAMILIES 16
#define MAX_GPU_MODELS 256
#define MAX_MODEL_INSTANCES 500000

#define MAX_WORLD_LINES 0xffff

typedef struct SolCamera
{
    vec3 position;
    vec3 target;
    float fov;
    float nearClip;
    float farClip;
    mat4 proj;
    mat4 view;
} SolCamera;

typedef struct SolVertex
{
    float position[3];
    float normal[3];
    float uv[2];
} SolVertex;

typedef struct SolTri
{
    vec3s a, b, c;
    vec3s normal;
    vec3s center;
    float boundRadius;
} SolTri;

typedef struct SolMaterial
{
    float baseColor[4];
    float metallic;
    float roughness;
} SolMaterial;

typedef struct SolLine
{
    vec3s a, b;
    vec3s aColor, bColor;
    float ttl;
} SolLine;

typedef struct WorldLines
{
    SolLine lines[MAX_WORLD_LINES];
    int count;
} WorldLines;

typedef struct SolMesh
{
    uint32_t vertexOffset;
    uint32_t vertexCount;
    uint32_t indexOffset;
    uint32_t indexCount;
    SolMaterial material;
} SolMesh;

typedef struct SolModel
{
    SolVertex *vertices;
    SolMesh *meshes;
    SolTri *tris;
    u32 *indices;

    u32 vertex_count;
    u32 mesh_count;
    u32 tri_count;
    u32 indice_count;
} SolModel;

typedef struct SolCollision
{
    bool didCollide;
    int id;
    vec3s pos, normal, vel;
} SolCollision;

typedef struct SpatialCell
{
    int ix, iy, iz;
    u32 neighborHashes[27];
} SpatialCell;

typedef struct SpatialTable
{
    u32 *head;
    u32 *value;
    u32 *next;
    u32 capacity;
    u32 size;
    u32 count;
} SpatialTable;

typedef struct SpatialGrid
{
    u32 *offsets; // [cellCount + 1] — start index per cell
    u32 *tris;    // [totalEntries] — sorted triangle indices
    u32 build_tri_count;
    float cellSize;
    vec3s min;
    vec3s max;
    ivec3s dims;
} SpatialGrid;

typedef struct WorldSpatial
{
    SpatialTable table_static;
    SpatialTable table_dynamic;

    SpatialGrid grid_static;
    SpatialGrid grid_dynamic;

    SolTri *tris_static;
    int tris_static_count;

    SolTri *tris_dynamic;
    int tris_dynamic_count;
} WorldSpatial;
