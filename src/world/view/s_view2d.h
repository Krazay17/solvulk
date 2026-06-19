#pragma once
#include "base.h"

#define MAX_VIEWS 8

typedef struct World World;

typedef enum
{
    VIEW2DKIND_RECT,
    VIEW2DKIND_TEXT,
    VIEW2DKIND_CIRCLE,
    VIEW2DKIND_COUNT,
} View2dKind;
typedef struct
{
    View2dKind kind;
    vec4s      dims, offset;
    vec4s      color;
    vec4s      hoverColor, clickColor, toggleColor;
    float      fill, scale, textWidth, border;
    float      hoverAnim, clickAnim;
    float      targetFill, fillSpeed;
    u32        zindex;
    u8         textureID, flags;
    vec2s      textureUV;
    char       text[64];
} SolView2d;
typedef struct CompView2d
{
    SolView2d views[MAX_VIEWS];
    u8        count;
    u8        zindex;
} CompView2d;

void        Sol_View2d_Init(World *world);
SolView2d  *Sol_View2d_Add(World *world, int id, View2dKind kind, vec4s color, float width, float height);
CompView2d *Sol_View2d_Get(World *world, int id);
void        Sol_View2d_Set(World *world, int id, CompView2d view);
void        Sol_View2d_SetText(World *world, int id, SolView2d *view, const char *text);
