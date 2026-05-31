#include "sol_core.h"

void Sol_Render_DrawText3D(Text3DDesc desc)
{
    SolFont     *font  = Sol_GetFont(desc.font);
    SolTextureId atlas = font->textureId;

    // Compute total width for centering (optional)
    float totalWidth = 0;
    for (const char *c = desc.text; *c; c++)
    {
        if (*c < 0 || *c >= 128)
            continue;
        totalWidth += font->glyph[(int)*c].yadvance;
    }

    float cursorX = -totalWidth * 0.5f; // center horizontally; remove for left-align

    // For billboard mode, get camera right/up vectors so all chars billboard together.
    vec3s right = {{1, 0, 0}};
    vec3s up    = {{0, 1, 0}};
    if (desc.billboard)
    {        
        // Extract from scene.view (or compute from camera)
        // For now, assume CPU has access to camera transform.
        // Sol_Cam_GetRight(&right);
        // Sol_Cam_GetUp(&up);
    }

    for (const char *c = desc.text; *c; c++)
    {
        if (*c < 0 || *c >= 128)
            continue;
        SolGlyph *g = &font->glyph[(int)*c];

        // Glyph world position
        float gx = cursorX + g->xoffset;
        float gy = g->yoffset;
        float gw = g->uw / font->atlasUscale; // glyph width in world units, however you scale
        float gh = g->vh / font->atlasVscale;

        // Compute glyph center in 3D
        vec3s glyphPos;
        if (desc.billboard)
        {
            glyphPos = vecAdd(desc.pos, vecAdd(vecSca(right, (gx + gw * 0.5f) * desc.size),
                                               vecSca(up, (gy + gh * 0.5f) * desc.size)));
        }
        else
        {
            vec3s local = {{(gx + gw * 0.5f) * desc.size, (gy + gh * 0.5f) * desc.size, 0}};
            glm_quat_rotatev(desc.rotation.raw, local.raw, glyphPos.raw);
            glyphPos = vecAdd(desc.pos, glyphPos);
        }

        QuadSSBO *q = Sol_Render_GetNext_Quad(QUADKIND_TEXT);
        if (!q)
            break;

        q->pos       = (vec4s){glyphPos.x, glyphPos.y, glyphPos.z, fmaxf(gw, gh) * 0.5f * desc.size};
        q->rot       = desc.billboard ? (vec4s){0, 0, 0, 0} // facecam mode: rot.x is spin angle (0)
                                      : (vec4s){desc.rotation.x, desc.rotation.y, desc.rotation.z, desc.rotation.w};
        q->color     = (vec4s){desc.color.r, desc.color.g, desc.color.b, desc.color.a};
        q->uv        = (vec4s){g->u, g->v, g->uw, g->vh};
        q->type      = desc.billboard ? QUADTYPE_FACECAM : QUADTYPE_QUAT;
        q->textureId = atlas;

        cursorX += g->yadvance;
    }
}