#pragma once
#include "sol/types.h"

typedef enum
{
    SOL_FONT_ICE,
    SOL_FONT_COUNT,
} SolFontKind;

typedef struct
{
    const char *str;
    float       x, y, size;
    vec4s       color;
    SolFontKind kind;
} SolFontDesc;

typedef struct
{
    float u, v, uw, vh;
    float xoffset;
    float ytop;
    float yoffset;
    float yadvance;
} SolGlyph;

typedef struct
{
    float l, b, r, t;
} TextBounds;

typedef struct SolFont
{
    SolGlyph     glyph[128];
    TextBounds   bounds;
    SolTextureId textureId;
    float        atlasUscale, atlasVscale;
} SolFont;

SolFont *Sol_GetFont(SolFontKind kind);
float    Sol_MeasureText(const char *str, float size, SolFontKind id);