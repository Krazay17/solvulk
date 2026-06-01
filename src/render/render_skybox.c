#include "sol_core.h"

#include "render_i.h"

#include "render/vk/vkrender.h"

static SolTextureId skybox_texture;

void Sol_Render_SkyboxSet(SolTextureId textureId)
{
    skybox_texture = textureId;
}

void Sol_Render_DrawSkybox()
{
    VkCommandBuffer cmd = Command_Buffer_Get();
    Bind_Pipeline(cmd, PIPE_SKYBOX);
    vkCmdDraw(cmd, 3, 1, 0, 0);
}