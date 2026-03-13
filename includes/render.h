#pragma once
#include <stdint.h>
#include <windows.h>

typedef struct
{
    float pos[3];
    float norm[3];
    float uv[2];
} Vert;

typedef struct
{
    Vert *verts;
    uint32_t VertCount;
    uint32_t *indices;
    uint32_t indexCount;
} Mesh;

typedef struct
{
    Mesh *mesh;
    uint32_t meshCount;
} Model;

void Sol_Init_Vulkan(HWND hwnd, HINSTANCE hInstance);
void Sol_Begin_Draw();
void Sol_End_Draw();
void Sol_DrawTriangle();