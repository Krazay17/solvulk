#include "render.h"
#include "render_internal.h"

#include "render/vk/vkrender.h"

void Render_Draw_Line(SolLine *lines, int count)
{
    SolFrameBufferRef ref = Sol_GetFrameBuffer(FRAMEBUFFER_LINE);
    SolLineVertex *verts = (SolLineVertex *)ref.mapped;
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
