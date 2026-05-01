#include "sol_core.h"
#include "font.h"
#include "../render/render.h"

void Load_Font(SolBank *bank)
{
    
    for (int i = 0; i < SOL_FONT_COUNT; i++)
    {
        SolResource fontIceMetrics = Sol_LoadResource(fontResourceName[i][0]);
        SolResource fontIceAtlas   = Sol_LoadResource(fontResourceName[i][1]);
        if (!fontIceAtlas.data || !fontIceMetrics.data)
            continue;

        Sol_UploadImage(fontIceAtlas.data, 224, 224, VK_FORMAT_R8G8B8A8_UNORM, SOL_IMAGE_FONT);
        Parse_Font_Metrics(fontIceMetrics.data, 224.0f, 224.0f, bank->fonts[SOL_FONT_ICE].glyph);
    }
}

void Parse_Font_Metrics(const char *json, float atlasW, float atlasH, SolGlyph *glyphs)
{
    const char *p = strstr(json, "\"glyphs\":[");
    if (!p)
        return;
    p += 10;

    while (*p)
    {
        const char *obj = strchr(p, '{');
        if (!obj)
            break;
        p = obj + 1;

        // find end of glyph block
        const char *end   = p;
        int         depth = 1;
        while (*end && depth > 0)
        {
            if (*end == '{')
                depth++;
            if (*end == '}')
                depth--;
            end++;
        }

        const char *uni = strstr(p, "\"unicode\":");
        if (!uni || uni > end)
        {
            p = end;
            continue;
        }
        int charId = (int)strtol(uni + 10, NULL, 10);
        if (charId < 0 || charId >= 128)
        {
            p = end;
            continue;
        }

        const char *adv   = strstr(p, "\"advance\":");
        const char *plane = strstr(p, "\"planeBounds\":");
        const char *atlas = strstr(p, "\"atlasBounds\":");

        TextBounds pl = plane && plane < end ? ParseBounds(plane, end) : (TextBounds){0};
        TextBounds ab = atlas && atlas < end ? ParseBounds(atlas, end) : (TextBounds){0};

        glyphs[charId].u        = ab.l / atlasW;
        glyphs[charId].v        = ab.b / atlasH;
        glyphs[charId].uw       = (ab.r - ab.l) / atlasW;
        glyphs[charId].vh       = (ab.t - ab.b) / atlasH;
        glyphs[charId].xoffset  = pl.l;
        glyphs[charId].ytop     = pl.t;
        glyphs[charId].yoffset  = pl.b;
        glyphs[charId].yadvance = adv ? strtof(adv + 10, NULL) : 0.0f;

        p = end;
    }
}

TextBounds ParseBounds(const char *p, const char *end)
{
    TextBounds  bounds = {0};
    const char *open   = strchr(p, '{');
    const char *close  = open ? strchr(open, '}') : NULL;
    if (!open || !close || open > end)
        return bounds;

    const char *kl = strstr(open, "\"left\":");
    const char *kb = strstr(open, "\"bottom\":");
    const char *kr = strstr(open, "\"right\":");
    const char *kt = strstr(open, "\"top\":");

    if (kl && kl < close)
        bounds.l = strtof(kl + 7, NULL);
    if (kb && kb < close)
        bounds.b = strtof(kb + 9, NULL);
    if (kr && kr < close)
        bounds.r = strtof(kr + 8, NULL);
    if (kt && kt < close)
        bounds.t = strtof(kt + 6, NULL);
    return bounds;
}


float Sol_MeasureText(const char *str, float size, SolFontId fontId)
{
    float width = 0.0f;
    for (const char *c = str; *c; c++)
    {
        int id = (int)*c;
        if (id < 0 || id >= 128)
            continue;
        width += Sol_Getbank()->fonts[fontId].glyph[id].yadvance * size;
    }
    return width;
}
