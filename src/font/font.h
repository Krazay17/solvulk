#pragma once
#include "render/render_types.h"
#include "sol/types.h"

typedef struct SolBank SolBank;

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

void       Parse_Font_Metrics(const char *json, float atlasW, float atlasH, SolGlyph *glyphs);
TextBounds ParseBounds(const char *p, const char *end);

void Sol_Draw_Text(const char *str, float x, float y, float size, vec4s color, SolFontId fontId);
