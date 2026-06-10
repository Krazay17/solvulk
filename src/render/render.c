/*
 * File: render.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-06-10
 * 
*/
#include "sol_core.h"

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