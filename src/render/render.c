/*
 * File: render.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-06-10
 *
 */
#include "sol_core.h"
#include "sol_math.h"
#include "render_i.h"
#include "render/vk/vkrender.h"

void Sol_Render_Resize(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0)
        return;
    Sol_Render_SetOrtho(width, height);
    Remake_Swapchain(width, height);
}

void Sol_Render_Flush3D(void)
{
    Flush_Models();
    Flush_Spheres();
    Flush_Quads();
    Flush_Ribbons();
}

void Sol_Render_Flush2D(void)
{
    Flush_Rects();
    Flush_Fonts2d();
}

void Sol_Render_DrawSkybox()
{
    VkCommandBuffer cmd = Command_Buffer_Get();
    Bind_Pipeline(cmd, PIPE_SKYBOX);
    vkCmdDraw(cmd, 3, 1, 0, 0);
}

void Sol_Render_DrawLine(SolLine *lines, int count)
{
    SolFrameBufferRef ref   = Sol_GetFrameBuffer(FRAMEBUFFER_LINE);
    SolLineVertex    *verts = (SolLineVertex *)ref.mapped;
    for (int i = 0; i < count; i++)
    {
        verts[i * 2 + 0] = (SolLineVertex){.pos = lines[i].a, .color = lines[i].aColor};
        verts[i * 2 + 1] = (SolLineVertex){.pos = lines[i].b, .color = lines[i].bColor};
    }

    VkCommandBuffer cmd = Command_Buffer_Get();
    Bind_Pipeline(cmd, PIPE_LINE);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, ref.buffers, &offset);

    vkCmdDraw(cmd, count * 2, 1, 0, 0);
}

ModelSubmission        modelQueue;
ModelSkinnedSubmission skinningQueue;

SphereQueue   sphereQueue;
SphereQueue   sphereFxQueue;
FireballQueue fireballQueue;
RibbonQueue   ribbonQueue;
RibbonQueue   ribbonQueueAdd;
RibbonQueue   ribbonQueueFront;

QuadQueue healthQueue;
QuadQueue spriteQueue0;
QuadQueue spriteQueue1;
QuadQueue spriteQueueFront;
QuadQueue text3dQueue;
QuadQueue text3dFrontQueue;

RectInstance rectQueue;
FontInstance font2dQueue;

void Flush_Models(void)
{
    // Always reset the frame allocation tracker to 0 at the start of flushing
    u32 model_que_offset = 0;

    // Early exit only if both queues are empty
    if (modelQueue.count == 0 && skinningQueue.count == 0)
        return;

    ModelSSBO *modelGpu = Sol_GetDescriptorMapping(DESC_MODEL_SSBO);

    // ==========================================
    // 1. STATIC MODELS PASS
    // ==========================================
    if (modelQueue.count > 0)
    {
        // Count per handle
        uint32_t counts[SOL_MODEL_COUNT] = {0};
        for (int i = 0; i < modelQueue.count; i++)
            counts[modelQueue.handles[i]]++;

        // Prefix sum
        uint32_t offsets[SOL_MODEL_COUNT] = {0};
        for (int i = 1; i < SOL_MODEL_COUNT; i++)
            offsets[i] = offsets[i - 1] + counts[i - 1];

        // Write sorted into SSBO
        uint32_t cursors[SOL_MODEL_COUNT];
        memcpy(cursors, offsets, sizeof(offsets));

        for (int i = 0; i < modelQueue.count; i++)
        {
            SolModelKind h = modelQueue.handles[i];
            // Since model_que_offset is 0 here, we map straight to the buffer start
            modelGpu[cursors[h]] = modelQueue.modelSSBO[i];
            cursors[h]++;
        }

        // Dispatch commands
        for (int h = 0; h < SOL_MODEL_COUNT; h++)
        {
            if (counts[h] > 0)
            {
                Render_Model(h, counts[h], offsets[h]);
            }
        }

        // Shift forward by static model footprint
        model_que_offset += modelQueue.count;
        modelQueue.count = 0;
    }

    // ==========================================
    // 2. SKINNED MODELS PASS
    // ==========================================
    if (skinningQueue.count > 0)
    {
        BonesSSBO *boneGpu = Sol_GetDescriptorMapping(DESC_SKINNING_SSBO);

        // Count per handle
        uint32_t counts[SOL_MODEL_COUNT] = {0};
        for (int i = 0; i < skinningQueue.count; i++)
            counts[skinningQueue.handles[i]]++;

        // Prefix sum (Local to skinning allocation space)
        uint32_t offsets[SOL_MODEL_COUNT] = {0};
        for (int i = 1; i < SOL_MODEL_COUNT; i++)
            offsets[i] = offsets[i - 1] + counts[i - 1];

        uint32_t cursors[SOL_MODEL_COUNT];
        memcpy(cursors, offsets, sizeof(offsets));

        for (int i = 0; i < skinningQueue.count; i++)
        {
            SolModelKind h = skinningQueue.handles[i];

            // GLOBAL INDEX = Current Queue Offset + Local Sorted Bucket Index
            uint32_t globalIdx = model_que_offset + cursors[h];

            // Write both model params and transformations to mirrored index offsets
            modelGpu[globalIdx] = skinningQueue.modelSSBO[i];
            boneGpu[globalIdx]  = skinningQueue.bones[i];

            cursors[h]++;
        }

        // Dispatch commands
        for (int h = 0; h < SOL_MODEL_COUNT; h++)
        {
            if (counts[h] > 0)
            {
                // Push draw call offset forward into the buffer using model_que_offset
                Render_Model_Skinned(h, counts[h], model_que_offset + offsets[h]);
            }
        }

        // Clean up allocation pointers
        model_que_offset += skinningQueue.count;
        skinningQueue.count = 0;
    }
}

void Flush_Spheres(void)
{
    SphereSSBO     *gpu = Sol_GetDescriptorMapping(DESC_SPHERE_SSBO);
    VkCommandBuffer cmd = Command_Buffer_Get();

    u32 currentOffset = 0;

    // 1. Regular Spheres
    u32 solidCount = sphereQueue.count;
    if (solidCount > 0)
    {
        memcpy(gpu + currentOffset, sphereQueue.instances, sizeof(SphereSSBO) * solidCount);
        Bind_Pipeline(cmd, PIPE_SPHERE);
        vkCmdDraw(cmd, 6, solidCount, 0, currentOffset);
        currentOffset += solidCount;
        sphereQueue.count = 0;
    }

    // 2. FX Spheres
    u32 fxCount = sphereFxQueue.count;
    if (fxCount > 0)
    {
        memcpy(gpu + currentOffset, sphereFxQueue.instances, sizeof(SphereSSBO) * fxCount);
        Bind_Pipeline(cmd, PIPE_SPHERE_FX);
        vkCmdDraw(cmd, 6, fxCount, 0, currentOffset);
        currentOffset += fxCount;
        sphereFxQueue.count = 0;
    }

    // 3. Fireballs
    u32 fireballCount = fireballQueue.count;
    if (fireballCount > 0)
    {
        memcpy(gpu + currentOffset, fireballQueue.instances, sizeof(SphereSSBO) * fireballCount);
        Bind_Pipeline(cmd, PIPE_FIREBALL);
        vkCmdDraw(cmd, 6, fireballCount, 0, currentOffset);
        currentOffset += fireballCount;
        fireballQueue.count = 0;
    }
}

void Flush_Quads()
{
    QuadSSBO       *gpu           = Sol_GetDescriptorMapping(DESC_QUAD_SSBO);
    VkCommandBuffer cmd           = Command_Buffer_Get();
    u32             currentOffset = 0;

    u32 healthCount = healthQueue.count;
    if (healthCount > 0)
    {
        memcpy(gpu + currentOffset, healthQueue.instances, sizeof(QuadSSBO) * healthCount);
        Bind_Pipeline(cmd, PIPE_HEALTHBAR);
        vkCmdDraw(cmd, 6, healthCount, 0, currentOffset);
        currentOffset += healthCount;
        if (currentOffset >= MAX_QUAD_INSTANCES)
            currentOffset = MAX_QUAD_INSTANCES - sizeof(QuadSSBO);
        healthQueue.count = 0;
    }

    u32 spriteCount0 = spriteQueue0.count;
    if (spriteCount0 > 0)
    {
        memcpy(gpu + currentOffset, spriteQueue0.instances, sizeof(QuadSSBO) * spriteCount0);
        Bind_Pipeline(cmd, PIPE_SPRITE);
        vkCmdDraw(cmd, 6, spriteCount0, 0, currentOffset);
        currentOffset += spriteCount0;
        if (currentOffset >= MAX_QUAD_INSTANCES)
            currentOffset = MAX_QUAD_INSTANCES - sizeof(QuadSSBO);
        spriteQueue0.count = 0;
    }

    u32 spriteCount1 = spriteQueue1.count;
    if (spriteCount1 > 0)
    {
        memcpy(gpu + currentOffset, spriteQueue1.instances, sizeof(QuadSSBO) * spriteCount1);
        Bind_Pipeline(cmd, PIPE_SPRITE_ADD);
        vkCmdDraw(cmd, 6, spriteCount1, 0, currentOffset);
        currentOffset += spriteCount1;
        if (currentOffset >= MAX_QUAD_INSTANCES)
            currentOffset = MAX_QUAD_INSTANCES - sizeof(QuadSSBO);
        spriteQueue1.count = 0;
    }

    if (spriteQueueFront.count > 0)
    {
        memcpy(gpu + currentOffset, spriteQueueFront.instances, sizeof(QuadSSBO) * spriteQueueFront.count);
        Bind_Pipeline(cmd, PIPE_SPRITE_FRONT);
        vkCmdDraw(cmd, 6, spriteQueueFront.count, 0, currentOffset);
        currentOffset += spriteQueueFront.count;
        if (currentOffset >= MAX_QUAD_INSTANCES)
            currentOffset = MAX_QUAD_INSTANCES - sizeof(QuadSSBO);
        spriteQueueFront.count = 0;
    }

    u32 textCount = text3dQueue.count;
    if (textCount > 0)
    {
        memcpy(gpu + currentOffset, text3dQueue.instances, sizeof(QuadSSBO) * textCount);
        Bind_Pipeline(cmd, PIPE_TEXT_3D);
        vkCmdDraw(cmd, 6, textCount, 0, currentOffset);
        currentOffset += textCount;
        if (currentOffset >= MAX_QUAD_INSTANCES)
            currentOffset = MAX_QUAD_INSTANCES - sizeof(QuadSSBO);
        text3dQueue.count = 0;
    }

    u32 textFrontCount = text3dFrontQueue.count;
    if (textFrontCount > 0)
    {
        memcpy(gpu + currentOffset, text3dFrontQueue.instances, sizeof(QuadSSBO) * textFrontCount);
        Bind_Pipeline(cmd, PIPE_TEXT_3D_FRONT);
        vkCmdDraw(cmd, 6, textFrontCount, 0, currentOffset);
        currentOffset += textFrontCount;
        if (currentOffset >= MAX_QUAD_INSTANCES)
            currentOffset = MAX_QUAD_INSTANCES - sizeof(QuadSSBO);
        text3dFrontQueue.count = 0;
    }
}

void Flush_Rects()
{
    if (rectQueue.count == 0)
        return;
    RectSSBO       *gpu = Sol_GetDescriptorMapping(DESC_RECT_SSBO);
    VkCommandBuffer cmd = Command_Buffer_Get();

    memcpy(gpu, rectQueue.instances, sizeof(RectSSBO) * rectQueue.count);
    Bind_Pipeline(cmd, PIPE_RECTI);
    vkCmdDraw(cmd, 6, rectQueue.count, 0, 0);
    rectQueue.count = 0;
}

void Flush_Fonts2d()
{
    if (font2dQueue.count == 0)
        return;
    FontSSBO       *gpu = Sol_GetDescriptorMapping(DESC_FONT_SSBO);
    VkCommandBuffer cmd = Command_Buffer_Get();

    memcpy(gpu, font2dQueue.instances, sizeof(FontSSBO) * font2dQueue.count);
    Bind_Pipeline(cmd, PIPE_TEXT_2D);
    vkCmdDraw(cmd, 6, font2dQueue.count, 0, 0);
    font2dQueue.count = 0;
}

void Flush_Ribbons()
{
    if (ribbonQueue.count == 0 && ribbonQueueFront.count == 0 && ribbonQueueAdd.count == 0)
        return;
    RibbonSegSSBO  *gpu           = Sol_GetDescriptorMapping(DESC_RIBBON_SSBO);
    VkCommandBuffer cmd           = Command_Buffer_Get();
    u32             currentOffset = 0;

    u32 regularCount = ribbonQueue.count;
    if (regularCount > 0)
    {
        memcpy(gpu + currentOffset, ribbonQueue.instances, sizeof(RibbonSegSSBO) * regularCount);
        Bind_Pipeline(cmd, PIPE_RIBBON);

        vkCmdDraw(cmd, 6, regularCount, 0, currentOffset);
        currentOffset += regularCount;

        ribbonQueue.count = 0;
    }

    u32 addCount = ribbonQueueAdd.count;
    if (addCount > 0)
    {
        memcpy(gpu + currentOffset, ribbonQueueAdd.instances, sizeof(RibbonSegSSBO) * addCount);
        Bind_Pipeline(cmd, PIPE_RIBBON_ADD);

        vkCmdDraw(cmd, 6, addCount, 0, currentOffset);
        currentOffset += addCount;

        ribbonQueueAdd.count = 0;
    }

    u32 frontCount = ribbonQueueFront.count;
    if (frontCount > 0)
    {
        memcpy(gpu + currentOffset, ribbonQueueFront.instances, sizeof(RibbonSegSSBO) * frontCount);
        Bind_Pipeline(cmd, PIPE_RIBBON_FRONT);
        vkCmdDraw(cmd, 6, frontCount, 0, currentOffset);
        currentOffset += frontCount;
        ribbonQueueFront.count = 0;
    }
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

        // OUTLINE
        QuadKind  quadKind = desc.inFront ? QUADKIND_TEXT_FRONT : QUADKIND_TEXT;
        QuadSSBO *q        = Sol_Render_GetNext_Quad(quadKind);
        if (!q)
            break;

        q->pos       = (vec4s){{glyphPos.x, glyphPos.y, glyphPos.z, 0}};
        q->rot       = desc.billboard ? (vec4s){{0, 0, 0, 0}} // FACECAM: rot.x = spin angle (0 = no spin)
                                      : (vec4s){{desc.rotation.x, desc.rotation.y, desc.rotation.z, desc.rotation.w}};
        q->color     = (vec4s){0,0,0,1.0f};
        q->uv        = (vec4s){{g->u, 1.0f - g->v - g->vh, g->uw, g->vh}};
        q->extra     = (vec4s){{0, 0, gw * 0.6f, gh * 0.54f}};
        q->type      = desc.billboard ? QUADTYPE_FACECAM : QUADTYPE_QUAT;
        q->textureId = atlas;
        q->flags     = 0;

        q        = Sol_Render_GetNext_Quad(quadKind);
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

void Sol_Render_DrawText2D(SolFontDesc desc)
{
    if (!desc.str || desc.str[0] == '\0')
        return;
    if (desc.size <= 0.0f)
        return;

    SolFont *font = Sol_GetFont(desc.kind);

    float cursorX  = desc.x;
    float baseSize = desc.size / 32.0f;
    float pad      = 0.2f * baseSize * (224.0f / 32.0f);

    for (const char *c = desc.str; *c; c++)
    {
        if (*c == ' ')
        {
            cursorX += font->glyph[' '].yadvance * desc.size;
            continue;
        }
        if ((unsigned char)*c >= 128)
            continue;

        SolGlyph *g = &font->glyph[(unsigned char)*c];
        if (g->uw == 0.0f)
            continue;

        FontSSBO *ssbo = Sol_Render_GetNext_Font();
        if (!ssbo)
            break;

        ssbo->pos   = (vec4s){{
            cursorX + g->xoffset * desc.size - pad,
            desc.y - g->ytop * desc.size - pad,
            g->uw * 224.0f * baseSize + pad * 2.0f,
            g->vh * 224.0f * baseSize + pad * 2.0f,
        }};
        ssbo->color = desc.color;
        ssbo->uv    = (vec4s){{
            g->u,
            1.0f - g->v - g->vh,
            g->uw,
            g->vh,
        }};

        cursorX += g->yadvance * desc.size;
    }
}