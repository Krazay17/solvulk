#pragma once
#include "sol/types.h"

#include "font.h"

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
    SolGlyph   glyph[128];
    TextBounds bounds;
} SolFont;

SolFont *Sol_GetFont(SolFontKind kind);