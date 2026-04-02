#pragma once
#include "sol/math.h"

typedef struct SolVkState SolVkState;

typedef enum
{
    SOL_MODEL_WIZARD,
    SOL_MODEL_WORLD0,
    SOL_MODEL_COUNT,
} SolModelId;

typedef struct
{
    vec3 position;
    vec3 target;
    float fov;
    float nearClip;
    float farClip;
    mat4 proj;
    mat4 view;
} SolCamera;

typedef struct
{
    float position[3];
    float normal[3];
    float uv[2];
} SolVertex;

typedef struct
{
    float baseColor[4];
    float metallic;
    float roughness;
} SolMaterial;

typedef struct
{
    SolVertex *vertices;
    uint32_t vertexCount;
    uint32_t *indices;
    uint32_t indexCount;
    SolMaterial material;
} SolMesh;

typedef struct SolModel
{
    SolMesh *meshes;
    uint32_t meshCount;
} SolModel;

extern SolCamera renderCam;

int Sol_Init_Vulkan(void *hwnd, void *hInstance);
void SolBindPipeline(int pipeIndex);

void Sol_Begin_Draw();
void Sol_Begin_3D();
void Sol_End_Draw();

void Sol_Render_Resize();
void Sol_SetOrtho(uint32_t width, uint32_t height);
void Sol_Camera_Update(vec3 pos, vec3 target);

void Sol_DrawModel(SolModelId handle, vec3 pos, float rotY);
void Sol_Draw_Model_Instanced(SolModelId handle, uint32_t instanceCount, uint32_t firstInstance);

void Sol_Draw_Rectangle(SolRect rect, SolColor color, float thickness);

void Sol_Draw_Text(const char *str, float x, float y, float size, SolColor color);
float Sol_MeasureText(const char *str, float size);
