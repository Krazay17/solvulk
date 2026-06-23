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
    u32         zindex;
} SolFontDesc;

typedef struct
{
    const char *text;
    vec3s       pos;
    float       size;
    vec4s       color;
    SolFontKind font;
    bool        billboard, inFront, outline; // face camera or use rotation
    versors     rotation;  // if not billboard
} Text3DDesc;

typedef struct
{
    float u, v, uw, vh;
    float xoffset;
    float xright;
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
} SolFont;

int      Sol_Fonts_Init();
SolFont *Sol_GetFont(SolFontKind kind);
float    Sol_MeasureText(const char *str, float size, SolFontKind id);