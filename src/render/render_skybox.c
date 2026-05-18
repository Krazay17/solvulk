#include "sol_core.h"

#include "render_i.h"

#include "render/vk/vkrender.h"

static SolTextureId skybox_texture;

void Sol_Render_SkyboxSet(World *world, SolTextureId textureId)
{
    skybox_texture = textureId;
}

void Flush_Skybox()
{
    if (!skybox_texture)
        return;
    VkCommandBuffer cmd = Command_Buffer_Get();
    Bind_Pipeline(cmd, PIPE_SKYBOX);
    vkCmdDraw(cmd, 3, 1, 0, 0); // 3 vertices, 1 instance
}