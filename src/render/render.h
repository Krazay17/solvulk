#pragma once
#include "sol/types.h"

typedef struct
{
    mat4 ortho2d;
} OrthoUBO;

typedef struct
{
    mat4 viewProjection;
    mat4 view;
    mat4 proj;
    vec4 cameraPos;
    vec4 sun;
} SceneUBO;

typedef struct
{
    vec4 position;
    vec4 scale;
    vec4 rotation;
    vec4 color;
    vec4 material;
} ModelSSBO;

typedef struct
{
    mat4 bones[MAX_BONES];
} SkinningSSBO;

typedef struct
{
    vec4 rec;
    vec4 c;
    vec4 extras;
} ShaderPushRect;

typedef struct
{
    vec4s pos; // xyz w=radius
    vec4s color;
} SphereSSBO;

typedef struct
{
    vec3s vertex;
} VertexSSBO;

typedef struct
{
    u32 flags;
} FlagsSSBO;

// void Render_Init();
// void Render_Push_Model(SolModelDraw model);
// void Render_Push_Model_Skinned(SolModelDraw model);
// void Render_Push_Sphere(SolSphere sphere);
// void Render_Push_Line(SolLine line);

// void Render_Draw_Model(SolModelDraw draw);
void Render_Draw_Model_Skinned();
void Render_Draw_Sphere();
void Render_Draw_Line();

void Render_3d();
void Render_2d();


void Sol_Draw_Sphere(vec4s pos, vec4s color);