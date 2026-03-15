#pragma once
#include <stdint.h>

typedef struct {
    float position[3];
    float normal[3];
    float uv[2];
} SolVertex;

typedef struct {
    SolVertex *vertices;
    uint32_t   vertexCount;
    uint32_t  *indices;
    uint32_t   indexCount;
} SolMesh;

typedef struct {
    SolMesh  *meshes;
    uint32_t  meshCount;
} SolModel;

// loads from embedded resource
SolModel SolLoadModel(const char *resourceName);
void SolFreeModel(SolModel *model);