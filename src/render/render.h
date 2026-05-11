#pragma once
#include "sol/types.h"

typedef enum
{
    DESC_ORTHO_UBO,
    DESC_SCENE_UBO,
    DESC_MODEL_SSBO,
    DESC_SKINNING_SSBO,
    DESC_FONT_ATLAS,
    DESC_BILLBOARD_SSBO,
    DESC_FLAGS_SSBO,
    DESC_COUNT,
} DescriptorId;

typedef enum
{
    FRAMEBUFFER_LINE,
    FRAMEBUFFER_COUNT,
}FrameBufferId;

typedef enum
{
    DESC_KIND_UBO,
    DESC_KIND_SSBO,
    DESC_KIND_IMAGE,
} DescriptorKind;

typedef enum PipelineId
{
    PIPE_MODEL,
    PIPE_MODEL_SKINNED,
    PIPE_TEXT,
    PIPE_LINE,
    PIPE_RECT,
    PIPE_BILLBOARD,
    PIPE_COUNT,
} PipelineId;

typedef enum
{
    BILLBOARD_SPHERE,
    BILLBOARD_HEALTHBAR,
    BILLBOARD_ICON,
    BILLBOARD_DAMAGE_NUMBER,
    BILLBOARD_PARTICLE,
} BillboardKind;

typedef struct
{
    BillboardKind kind;
    vec4s         pos;
    vec4s         color;  // tint, default white
    vec4s         params; // type-specific (fill amount, icon ID, etc.)
    u32           flags;
} BillboardDesc;

typedef struct
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

float Sol_Render_GetAspect(void);

// Single Draw rendering
void Render_Draw_Line(SolLine *lines, int count);
void Render_Draw_Rectangle(vec4s rect, vec4s color, float thickness);
void Sol_Render_Draw_Text(SolFontDesc desc);

// Instance rendering
void Sol_Render_PushBillboard(BillboardDesc desc);
void Sol_Render_PushModel(ModelPushDesc desc);
