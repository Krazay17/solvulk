#include "sol_core.h"

ShaderPushTexts Prepare_Text(SolFontDesc desc)
{
    const char *str   = desc.str;
    float       x     = desc.x;
    float       y     = desc.y;
    float       size  = desc.size;
    vec4s       color = desc.color;
    SolFontKind kind  = desc.kind;
    SolFont    *font  = Sol_GetFont(kind);
    u32         len   = (u32)strlen(str);

    static ShaderPushText push[256] = {0};

    ShaderPushTexts result = {
        .push  = push,
        .count = 0,
    };

    float cursorX  = x;
    float baseSize = size / 32.0f; // Calculate once outside the loop
    u32   index    = 0;
    for (const char *c = str; *c; c++)
    {
        if (*c == ' ')
        {
            cursorX += font->glyph[' '].yadvance * size;
            continue;
        }
        if ((unsigned char)*c >= 256)
            continue;

        SolGlyph *g = &font->glyph[(unsigned char)*c];
        // Ensure we are actually getting valid glyph data
        if (g->uw == 0)
            continue;

        float pad = 0.2f * baseSize * (224.0f / 32.0f);

        push[index].x = cursorX + g->xoffset * size - pad;
        push[index].y = y - g->ytop * size - pad;
        push[index].w = g->uw * 224.0f * baseSize + pad * 2.0f;
        push[index].h = g->vh * 224.0f * baseSize + pad * 2.0f;

        push[index].r = ColorConvert(color.r);
        push[index].g = ColorConvert(color.g);
        push[index].b = ColorConvert(color.b);
        push[index].a = ColorConvert(color.a);

        push[index].u  = g->u;
        push[index].v  = 1.0f - g->v - g->vh;
        push[index].uw = g->uw;
        push[index].vh = g->vh;

        index++;
        cursorX += g->yadvance * size;
    }

    result.count = index;
    return result;
}

float Sol_MeasureText(const char *str, float size, SolFontKind kind)
{
    float width = 0.0f;
    for (const char *c = str; *c; c++)
    {
        int id = (int)*c;
        if (id < 0 || id >= 128)
            continue;
        width += Sol_GetFont(kind)->glyph[id].yadvance * size;
    }
    return width;
}
