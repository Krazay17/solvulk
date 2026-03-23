#pragma once
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <stdatomic.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef struct
{
    atomic_bool isRunning;
    atomic_bool needsResize;
    float windowWidth, windowHeight;
    HWND g_hwnd;
} SolState;

extern SolState solState;

typedef struct {
    const void *data;
    size_t size;
} SolResource;

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

typedef struct SolModel {
    SolMesh  *meshes;
    uint32_t  meshCount;
} SolModel;

//Needs free
char* Sol_ReadFile(const char* filename, size_t* outSize);
// No free needed - memory is owned by the exe
SolResource Sol_LoadResource(const char *resourceName);

// loads from embedded resource
SolModel Sol_LoadModel(const char *resourceName);
void Sol_FreeModel(SolModel *model);

void Sol_Loader_LoadModels();