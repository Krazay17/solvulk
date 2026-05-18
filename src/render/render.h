#pragma once
#include "sol/types.h"

#include "font/font.h"

typedef struct SolCamera SolCamera;

typedef enum
{
    PIPE_MODEL,
    PIPE_MODEL_SKINNED,
    
    PIPE_TEXT,
    PIPE_RECT,
    PIPE_LINE,
    
    PIPE_SPHERE,
    PIPE_SPHERE_FX,
    PIPE_SPRITE,
    PIPE_SPRITE_FX,
    
    PIPE_SKYBOX,
    PIPE_BILLBOARD,

    PIPE_COUNT,
} PipelineId;

typedef struct SolLineVertex
{
    vec3s pos;
    vec4s color;
} SolLineVertex;

typedef struct
{
    vec4s pos;
    vec4s color;
    vec4s params;
    bool  isfx;
} SphereDesc;

typedef enum
{
    BILLBOARD_HEALTHBAR,
    BILLBOARD_ICON,
    BILLBOARD_DAMAGE_NUMBER,
} BillboardKind;
typedef struct
{
    BillboardKind kind;
    vec4s         pos;
    vec4s         color;
    vec4s         params;
    u32           flags;
} BillboardDesc;

typedef enum
{
    SPRITE_CAMFACE,
    SPRITE_3D,
} SpriteKind;
typedef struct
{
    vec4s        pos;
    versors      rotation;
    vec4s        color;
    vec4s        uv;
    SpriteKind   kind;
    SolTextureId textureId;
    bool         isfx;
} SpriteDesc;

typedef struct ModelPushDesc
{
    SolModelId handle;
    vec4s      position;
    vec4s      scale;
    vec4s      rotation;
    vec4s      color;
    vec4s      material;
    u32        flags;
    bool       hasAnim;
    mat4      *bones;
} ModelPushDesc;

void Sol_Render_Camera_Update(SolCamera *cam);
void Sol_Render_Resize(uint32_t width, uint32_t height);

void Sol_Render_PushSphere(SphereDesc desc);
void Sol_Render_PushBillboard(BillboardDesc desc);
void Sol_Render_PushSprite(SpriteDesc desc);
void Sol_Render_PushModel(ModelPushDesc desc);

float Sol_Render_GetAspect(void);
void  Sol_Render_DrawLine(SolLine *lines, int count);
void  Sol_Render_DrawRectangle(vec4s rect, vec4s color, float thickness);
void  Sol_Render_DrawText(SolFontDesc desc);

void Sol_Render_SkyboxSet(World *world, SolTextureId textureId);