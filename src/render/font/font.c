#include "sol_core.h"

const char *font_path[SOL_FONT_COUNT] = {
    [SOL_FONT_ICE] = {"atlas.json"},
};

SolFont loaded_fonts[SOL_FONT_COUNT];

static void       Parse_Font(SolResource metrics, int id);
static TextBounds ParseBounds(const char *p, const char *end);

int Sol_Fonts_Init()
{
    for (int i = 0; i < SOL_FONT_COUNT; i++)
    {
        SolResource res = Sol_LoadResource(font_path[i]);
        if (res.data)
            Parse_Font(res, i);
    }
    return 0;
}

SolFont *Sol_GetFont(SolFontKind kind)
{
    return &loaded_fonts[kind];
}

static void Parse_Font(SolResource metrics, int id)
{
    SolFont *font      = &loaded_fonts[id];
    font->textureId    = id;
    SolGlyph   *glyphs = font->glyph;
    const char *json   = metrics.data;

    float atlasW = 224.0f;
    float atlasH = 224.0f;

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
        glyphs[charId].xright   = pl.r;
        glyphs[charId].ytop     = pl.t;
        glyphs[charId].yoffset  = pl.b;
        glyphs[charId].yadvance = adv ? strtof(adv + 10, NULL) : 0.0f;

        p = end;
    }
}

static TextBounds ParseBounds(const char *p, const char *end)
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

        push[index].r = color.r;
        push[index].g = color.g;
        push[index].b = color.b;
        push[index].a = color.a;

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
void Sol_Render_DrawText3D(Text3DDesc desc)
{
    if (!desc.text || desc.text[0] == '\0')
        return;
    if (desc.size <= 0.0f)
        return;

    SolFont     *font  = Sol_GetFont(desc.font);
    SolTextureId atlas = font->textureId;

    // Total width in world units (advance * size) for horizontal centering
    float totalWidth = 0.0f;
    for (const char *c = desc.text; *c; c++)
    {
        unsigned char ch = (unsigned char)*c;
        if (ch >= 128)
            continue;
        totalWidth += font->glyph[ch].yadvance * desc.size;
    }

    // Layout basis vectors.
    // Billboard mode: use camera right + world up so all chars share the same axes
    //                 and stay on a line facing the camera.
    // Quat mode:      use local x/y axes; the rotation quaternion is applied per glyph in the shader.
    vec3s right = (vec3s){{1.0f, 0.0f, 0.0f}};
    vec3s up    = (vec3s){{0.0f, 1.0f, 0.0f}};
    if (desc.billboard)
    {
        right = Sol_Cam_GetRight();
        // up stays world-up so text stays vertically aligned with screen
    }

    float cursorX = -totalWidth * 0.5f; // centered horizontally

    for (const char *c = desc.text; *c; c++)
    {
        unsigned char ch = (unsigned char)*c;
        if (ch >= 128)
            continue;

        // Whitespace: advance cursor, no draw
        if (ch == ' ')
        {
            cursorX += font->glyph[' '].yadvance * desc.size;
            continue;
        }

        SolGlyph *g = &font->glyph[ch];
        if (g->uw == 0.0f)
        { // unknown / unloaded glyph
            cursorX += g->yadvance * desc.size;
            continue;
        }

        // --- Compute glyph quad in world units ---
        // Plane bounds give where to draw the glyph relative to its origin (in EM units).
        // Multiply by desc.size to convert EM to world units.
        float gw = (g->xright - g->xoffset) * desc.size; // glyph width
        float gh = (g->ytop - g->yoffset) * desc.size;   // glyph height

        // Glyph center (in 2D layout space, relative to text origin)
        float cx = cursorX + (g->xoffset + g->xright) * 0.5f * desc.size;
        float cy = (g->yoffset + g->ytop) * 0.5f * desc.size;

        // Project the 2D layout position into world space using the basis vectors.
        vec3s glyphPos;
        if (desc.billboard)
        {
            glyphPos = vecAdd(desc.pos, vecAdd(vecSca(right, cx), vecSca(up, cy)));
        }
        else
        {
            // Quat mode: rotate the local (cx, cy, 0) by the orientation, then offset from origin.
            vec3s local = {{cx, cy, 0.0f}};
            vec3s rotated;
            glm_quat_rotatev(desc.rotation.raw, local.raw, rotated.raw);
            glyphPos = vecAdd(desc.pos, rotated);
        }

        // --- Push quad ---
        // NOTE: your vertex shader uses a single `size = q.pos.w` for both axes,
        //       so quads are uniform-scaled squares. We use max(gw, gh)/2 as the
        //       half-size, which means narrow glyphs (I, l) get a square quad with
        //       the glyph stretched horizontally to fill it. For most text this is
        //       acceptable; for crisp typography, modify the vert shader to take a
        //       vec2 size from q.extra and pass non-uniform dimensions.
        // float halfSize = fmaxf(gw, gh) * 0.5f;

        QuadSSBO *q = Sol_Render_GetNext_Quad(QUADKIND_TEXT);
        if (!q)
            break;

        q->pos       = (vec4s){{glyphPos.x, glyphPos.y, glyphPos.z, 0}};
        q->rot       = desc.billboard ? (vec4s){{0, 0, 0, 0}} // FACECAM: rot.x = spin angle (0 = no spin)
                                      : (vec4s){{desc.rotation.x, desc.rotation.y, desc.rotation.z, desc.rotation.w}};
        q->color     = desc.color;
        q->uv        = (vec4s){{g->u, 1.0f - g->v - g->vh, g->uw, g->vh}};
        q->extra     = (vec4s){{0, 0, gw * 0.5f, gh * 0.5f}};
        q->type      = desc.billboard ? QUADTYPE_FACECAM : QUADTYPE_QUAT;
        q->textureId = atlas;
        q->flags     = 0;

        cursorX += g->yadvance * desc.size;
    }
}