#pragma once
#include "solmath.h"
#include "sol_load_types.h"

typedef struct SolModel SolModel;

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
    mat4 ortho;
    SolRect rect;
    SolColor color;
    vec4 extra;
} BatchedRects;

extern SolCamera renderCam;

int Sol_Init_Vulkan(void* hwnd, void* hInstance);

SolModelHandle Sol_UploadModel(SolModel *model);

void Sol_Begin_Draw();
void Sol_End_Draw();
void Sol_Render_Resize();

void Sol_Camera_Update(vec3 pos, vec3 target);
float Sol_MeasureText(const char *str, float size);

void Sol_DrawModel(SolModelHandle handle, vec3 pos, float rotY);

void Sol_Draw_Rectangle(SolRect rect, SolColor color, float thickness);
void Sol_Draw_Batch_Rectangle(BatchedRects *rects);

void Sol_Draw_Text(const char *str, float x, float y, float size, SolColor color);