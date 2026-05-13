#include "sol_core.h"

#include "model/model_i.h"
#include "render/render_i.h"
#include "texture/texture.h"
#include "vkrender.h"

static SolVkState solvkstate = {0};

static u32         boundPipeline;
static SolGpuImage gpuImages[SOL_TEXTURE_COUNT];
static SolGpuModel gpuModels[SOL_MODEL_COUNT];

static SolFrameBuffer frameBuffers[FRAMEBUFFER_COUNT];
static SolDescriptor  descriptors[DESC_COUNT];
static SolPipe        pipes[PIPE_COUNT];

static SolFrameBufferConfig buffer_config[FRAMEBUFFER_COUNT] = {
    [FRAMEBUFFER_LINE] =
        {
            .size  = sizeof(SolLineVertex) * MAX_LINE_VERTICES,
            .stage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        },
};

static SolPipelineConfig pipe_config[PIPE_COUNT] = {
    [PIPE_TEXT] =
        {
            .vertResource      = "ID_SHADER_TEXT_V",
            .fragResource      = "ID_SHADER_TEXT_F",
            .cullMode          = VK_CULL_MODE_NONE,
            .pushRangeSize     = sizeof(ShaderPushText),
            .pushStageFlags    = VK_SHADER_STAGE_VERTEX_BIT,
            .primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .descId            = {DESC_ORTHO_UBO, DESC_FONT_ATLAS},
            .descCount         = 2,
        },
    [PIPE_MODEL] =
        {
            .vertResource      = "ID_SHADER_MODEL_V",
            .fragResource      = "ID_SHADER_MODEL_F",
            .depthTest         = 1,
            .depthWrite        = 1,
            .blendMode         = BLEND_ALPHA,
            .cullMode          = VK_CULL_MODE_NONE,
            .pushRangeSize     = sizeof(SolMaterial),
            .pushStageFlags    = VK_SHADER_STAGE_FRAGMENT_BIT,
            .primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .type              = VERTEX_TRI,
            .descId            = {DESC_SCENE_UBO, DESC_MODEL_SSBO, DESC_FLAGS_SSBO},
            .descCount         = 3,
        },
    [PIPE_RECT] =
        {
            .vertResource      = "ID_SHADER_RECT_V",
            .fragResource      = "ID_SHADER_RECT_F",
            .blendMode         = BLEND_ALPHA,
            .cullMode          = VK_CULL_MODE_NONE,
            .pushRangeSize     = sizeof(ShaderPushRect),
            .pushStageFlags    = VK_SHADER_STAGE_VERTEX_BIT,
            .primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .descId            = {DESC_ORTHO_UBO},
            .descCount         = 1,
        },
    [PIPE_SPHERE] =
        {
            .vertResource      = "ID_SHADER_SPHERE_V",
            .fragResource      = "ID_SHADER_SPHERE_F",
            .depthTest         = 1,
            .depthWrite        = 1,
            .blendMode         = BLEND_ALPHA,
            .cullMode          = VK_CULL_MODE_BACK_BIT,
            .descId            = {DESC_SCENE_UBO, DESC_SPHERE},
            .descCount         = 2,
            .primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        },
    [PIPE_QUAD] =
        {
            .vertResource      = "ID_SHADER_QUAD_V",
            .fragResource      = "ID_SHADER_QUAD_F",
            .depthTest         = 1,
            .depthWrite        = 0,
            .blendMode         = BLEND_ADDITIVE,
            .cullMode          = VK_CULL_MODE_NONE,
            .descId            = {DESC_SCENE_UBO, DESC_QUAD, DESC_SPRITE},
            .descCount         = 3,
            .primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        },
    [PIPE_BILLBOARD] =
        {
            .vertResource      = "ID_SHADER_BILLBOARD_V",
            .fragResource      = "ID_SHADER_BILLBOARD_F",
            .depthTest         = 1,
            .depthWrite        = 1,
            .blendMode         = BLEND_ALPHA,
            .cullMode          = VK_CULL_MODE_BACK_BIT,
            .pushRangeSize     = sizeof(BillboardSSBO),
            .pushStageFlags    = VK_SHADER_STAGE_VERTEX_BIT,
            .primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .descId            = {DESC_SCENE_UBO, DESC_BILLBOARD_SSBO, DESC_FLAGS_SSBO},
            .descCount         = 3,
        },
    [PIPE_LINE] =
        {
            .vertResource      = "ID_SHADER_LINE_V",
            .fragResource      = "ID_SHADER_LINE_F",
            .depthTest         = 1,
            .blendMode         = BLEND_ALPHA,
            .cullMode          = VK_CULL_MODE_NONE,
            .pushRangeSize     = 0,
            .pushStageFlags    = 0,
            .type              = VERTEX_LINE,
            .primitiveTopology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
            .descId            = {DESC_SCENE_UBO},
            .descCount         = 1,
        },
    [PIPE_MODEL_SKINNED] =
        {
            .vertResource      = "ID_SHADER_SKINNED_V",
            .fragResource      = "ID_SHADER_MODEL_F",
            .descId            = {DESC_SCENE_UBO, DESC_MODEL_SSBO, DESC_FLAGS_SSBO, DESC_SKINNING_SSBO},
            .descCount         = 4,
            .type              = VERTEX_SKINNED,
            .depthTest         = 1,
            .depthWrite        = 1,
            .blendMode         = BLEND_ALPHA,
            .cullMode          = VK_CULL_MODE_NONE,
            .pushRangeSize     = sizeof(SolMaterial),
            .pushStageFlags    = VK_SHADER_STAGE_FRAGMENT_BIT,
            .primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        },
};

static SolDescriptorConfig desc_config[DESC_COUNT] = {
    [DESC_ORTHO_UBO] =
        {
            .size       = sizeof(OrthoUBO),
            .type       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .kind       = DESC_KIND_UBO,
        },
    [DESC_SCENE_UBO] =
        {
            .size       = sizeof(SceneUBO),
            .type       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            .kind       = DESC_KIND_UBO,
        },
    [DESC_MODEL_SSBO] =
        {
            .size       = sizeof(ModelSSBO) * MAX_MODEL_INSTANCES,
            .type       = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .kind       = DESC_KIND_SSBO,
        },
    [DESC_FONT_ATLAS] =
        {
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .kind       = DESC_KIND_IMAGE,
            .imageId    = SOL_TEXTURE_ICEFONT,
        },
    [DESC_SPHERE] =
        {
            .size       = sizeof(SphereSSBO) * MAX_SPHERE_INSTANCES,
            .type       = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .kind       = DESC_KIND_SSBO,
        },
    [DESC_QUAD] =
        {
            .size       = sizeof(QuadSSBO) * MAX_QUAD_INSTANCES,
            .type       = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT,
            .kind       = DESC_KIND_SSBO,
        },
    [DESC_SPRITE] =
        {
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .kind       = DESC_KIND_IMAGE,
            .imageId    = SOL_TEXTURE_GFLAME, // single texture for now
        },
    [DESC_BILLBOARD_SSBO] =
        {
            .size       = sizeof(BillboardSSBO) * MAX_BILLBOARD_INSTANCES,
            .type       = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            .kind       = DESC_KIND_SSBO,
        },
    [DESC_FLAGS_SSBO] =
        {
            .size       = sizeof(FlagsSSBO) * MAX_MODEL_INSTANCES,
            .type       = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            .kind       = DESC_KIND_SSBO,
        },
    [DESC_SKINNING_SSBO] =
        {
            .size       = sizeof(BonesSSBO) * MAX_MODEL_INSTANCES,
            .type       = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .kind       = DESC_KIND_SSBO,
        },

};

int Sol_Render_Init(void *hwnd, void *hInstance)
{
    if (SolVkInstance(&solvkstate) != 0)
        return 1;
    if (SolVkSurface(&solvkstate, hwnd, hInstance) != 0)
        return 2;
    if (SolVkPhysicalDevice(&solvkstate) != 0)
        return 3;
    if (SolVkDevice(&solvkstate) != 0)
        return 4;
    if (SolVkSwapchain(&solvkstate) != 0)
        return 5;
    if (SolVkImageViews(&solvkstate) != 0)
        return 6;
    if (SolVkDepthResources(&solvkstate) != 0)
        return 7;
    if (SolVkCommandPool(&solvkstate) != 0)
        return 8;
    if (SolVkSyncObjects(&solvkstate) != 0)
        return 9;
    if (Sol_Render_UploadAll() != 0)
        return 10;
    if (Sol_Render_BuildPipes() != 0)
        return 11;
    return 0;
}

int Sol_Render_UploadAll()
{
    for (int i = 0; i < SOL_TEXTURE_COUNT; i++)
    {
        Sol_UploadImage(Sol_GetImage(i), i);
    }

    for (int i = 0; i < SOL_MODEL_COUNT; i++)
    {
        Sol_UploadModel(Sol_GetModel(i), i);
    }
    return 0;
}

int Sol_Render_BuildPipes()
{
    for (int i = 0; i < DESC_COUNT; i++)
    {
        if (Sol_Descriptor_Build(&solvkstate, &desc_config[i], &descriptors[i]) != 0)
        {
            Sol_MessageBox("DESC ERROR", NULL);
            return 9;
        }
    }
    for (int i = 0; i < PIPE_COUNT; i++)
    {
        if (Sol_Pipeline_Build(&solvkstate, &pipe_config[i], &pipes[i]) != 0)
        {
            Sol_MessageBox("PIPE ERROR", NULL);
            return 10;
        }
    }

    for (int i = 0; i < FRAMEBUFFER_COUNT; i++)
    {
        Sol_CreateFrameBuffer(&solvkstate, buffer_config[i].size, buffer_config[i].stage,
                              &frameBuffers[FRAMEBUFFER_LINE]);
    }

    Sol_Render_SetOrtho(solvkstate.swapchainExtent.width, solvkstate.swapchainExtent.height);

    return 0;
}

VkCommandBuffer Command_Buffer_Get()
{
    return solvkstate.commandBuffers[solvkstate.currentFrame];
}

void *Sol_GetDescriptorMapping(DescriptorId id)
{
    return descriptors[id].mapped[solvkstate.currentFrame];
}

SolFrameBufferRef Sol_GetFrameBuffer(FrameBufferId id)
{
    u32 frame = solvkstate.currentFrame;
    return (SolFrameBufferRef){.buffers = &frameBuffers[id].buffers[frame], .mapped = frameBuffers[id].mapped[frame]};
}

float Sol_Render_GetAspect(void)
{
    if (solvkstate.swapchainExtent.height == 0)
        return 1.0f;
    return (float)solvkstate.swapchainExtent.width / (float)solvkstate.swapchainExtent.height;
}

void Bind_Pipeline(VkCommandBuffer cmd, PipelineId id)
{
    if (id == boundPipeline)
        return;

    SolPipe           *pipe = &pipes[id];
    SolPipelineConfig *cfg  = &pipe_config[id];

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe->pipeline);

    if (cfg->descCount > 0)
    {
        VkDescriptorSet sets[4];
        for (u32 i = 0; i < cfg->descCount; i++)
        {
            sets[i] = descriptors[cfg->descId[i]].sets[solvkstate.currentFrame];
        }
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe->layout, 0, cfg->descCount, sets, 0, NULL);
    }

    boundPipeline = id;
}

void Sol_Render_SetOrtho(uint32_t width, uint32_t height)
{
    mat4 ortho;
    glm_ortho(0.0f, width, 0.0f, height, -1.0f, 1.0f, ortho);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        OrthoUBO *ubo = descriptors[DESC_ORTHO_UBO].mapped[i];
        memcpy(ubo->ortho2d, ortho, sizeof(mat4));
    }
}

void Remake_Swapchain(uint32_t width, uint32_t height)
{
    vkDeviceWaitIdle(solvkstate.device);

    // destroy old
    vkDestroyImageView(solvkstate.device, solvkstate.depthImageView, NULL);
    vkDestroyImage(solvkstate.device, solvkstate.depthImage, NULL);
    vkFreeMemory(solvkstate.device, solvkstate.depthMemory, NULL);

    for (uint32_t i = 0; i < solvkstate.swapchainImageCount; i++)
        vkDestroyImageView(solvkstate.device, solvkstate.swapchainImageViews[i], NULL);

    vkDestroySwapchainKHR(solvkstate.device, solvkstate.swapchain, NULL);

    SolVkSwapchain(&solvkstate);
    SolVkImageViews(&solvkstate);
    SolVkDepthResources(&solvkstate);
}

void Sol_Render_DrawRectangle(vec4s rect, vec4s color, float thickness)
{
    VkCommandBuffer cmd = Command_Buffer_Get();
    Bind_Pipeline(cmd, PIPE_RECT);

    ShaderPushRect push = {
        .rec    = {rect.x, rect.y, rect.z, rect.w},
        .color  = {ColorConvert(color.r), ColorConvert(color.g), ColorConvert(color.b), ColorConvert(color.a)},
        .extras = {thickness, 0, 0, 0},
    };

    vkCmdPushConstants(cmd, pipes[PIPE_RECT].layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ShaderPushRect), &push);
    vkCmdDraw(cmd, 6, 1, 0, 0);
}

void Render_Model(SolModelId handle, uint32_t instanceCount, uint32_t firstInstance)
{
    VkCommandBuffer cmd = Command_Buffer_Get();
    Bind_Pipeline(cmd, PIPE_MODEL);

    SolGpuModel *model = &gpuModels[handle];
    for (uint32_t m = 0; m < model->mesh_count; m++)
    {
        vkCmdPushConstants(cmd, pipes[PIPE_MODEL].layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SolMaterial),
                           &model->meshes[m].material);
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmd, 0, 1, &model->meshes[m].vertexBuffer, offsets);
        vkCmdBindIndexBuffer(cmd, model->meshes[m].indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmd, model->meshes[m].indexCount, instanceCount, 0, 0, firstInstance);
    }
}

void Render_Model_Skinned(SolModelId handle, uint32_t instanceCount, uint32_t firstInstance)
{
    VkCommandBuffer cmd = Command_Buffer_Get();
    Bind_Pipeline(cmd, PIPE_MODEL_SKINNED);

    SolGpuModel *model = &gpuModels[handle];
    for (uint32_t m = 0; m < model->mesh_count; m++)
    {
        vkCmdPushConstants(cmd, pipes[PIPE_MODEL_SKINNED].layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SolMaterial),
                           &model->meshes[m].material);
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmd, 0, 1, &model->meshes[m].vertexBuffer, offsets);
        vkCmdBindIndexBuffer(cmd, model->meshes[m].indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmd, model->meshes[m].indexCount, instanceCount, 0, 0, firstInstance);
    }
}

void Sol_Render_DrawText(SolFontDesc desc)
{
    VkCommandBuffer cmd = Command_Buffer_Get();
    Bind_Pipeline(cmd, PIPE_TEXT);

    ShaderPushTexts texts = Prepare_Text(desc);
    for (int i = 0; i < texts.count; i++)
    {
        vkCmdPushConstants(cmd, pipes[PIPE_TEXT].layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ShaderPushText),
                           (texts.push + i));
        vkCmdDraw(cmd, 6, 1, 0, 0);
    }
}

void Sol_Begin_Draw()
{
    boundPipeline = -1;
    vkWaitForFences(solvkstate.device, 1, &solvkstate.inFlightFences[solvkstate.currentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(solvkstate.device, 1, &solvkstate.inFlightFences[solvkstate.currentFrame]);

    vkAcquireNextImageKHR(solvkstate.device, solvkstate.swapchain, UINT64_MAX,
                          solvkstate.imageAvailableSemaphores[solvkstate.currentFrame], VK_NULL_HANDLE,
                          &solvkstate.currentImageIndex);

    VkCommandBuffer currentCmd = solvkstate.commandBuffers[solvkstate.currentFrame];
    vkResetCommandBuffer(currentCmd, 0);

    VkCommandBufferBeginInfo beginInfo = {0};
    beginInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(currentCmd, &beginInfo);

    VkImageMemoryBarrier depthBarrier        = {0};
    depthBarrier.sType                       = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    depthBarrier.oldLayout                   = VK_IMAGE_LAYOUT_UNDEFINED;
    depthBarrier.newLayout                   = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depthBarrier.srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
    depthBarrier.dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
    depthBarrier.image                       = solvkstate.depthImage;
    depthBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    depthBarrier.subresourceRange.levelCount = 1;
    depthBarrier.subresourceRange.layerCount = 1;
    depthBarrier.srcAccessMask               = 0;
    depthBarrier.dstAccessMask               = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    vkCmdPipelineBarrier(currentCmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 0,
                         0, NULL, 0, NULL, 1, &depthBarrier);

    // clear depth
    VkClearValue depthClear       = {0};
    depthClear.depthStencil.depth = 1.0f;

    VkRenderingAttachmentInfo depthAttachment = {0};
    depthAttachment.clearValue                = depthClear;
    depthAttachment.sType                     = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.imageView                 = solvkstate.depthImageView;
    depthAttachment.imageLayout               = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp                    = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp                   = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    // transition → color attachment
    VkImageMemoryBarrier toRender        = {0};
    toRender.sType                       = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    toRender.oldLayout                   = VK_IMAGE_LAYOUT_UNDEFINED;
    toRender.newLayout                   = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    toRender.srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
    toRender.dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
    toRender.image                       = solvkstate.swapchainImages[solvkstate.currentImageIndex];
    toRender.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    toRender.subresourceRange.levelCount = 1;
    toRender.subresourceRange.layerCount = 1;
    toRender.srcAccessMask               = 0;
    toRender.dstAccessMask               = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    vkCmdPipelineBarrier(currentCmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         0, 0, NULL, 0, NULL, 1, &toRender);

    VkClearValue clearColor = {{RENDER_CLEAR_COLOR}};

    VkRenderingAttachmentInfo colorAttachment = {0};
    colorAttachment.sType                     = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView                 = solvkstate.swapchainImageViews[solvkstate.currentImageIndex];
    colorAttachment.imageLayout               = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp                    = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp                   = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue                = clearColor;

    VkRenderingInfo renderingInfo      = {0};
    renderingInfo.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.extent    = solvkstate.swapchainExtent;
    renderingInfo.layerCount           = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments    = &colorAttachment;
    renderingInfo.pDepthAttachment     = &depthAttachment;

    vkCmdBeginRendering(currentCmd, &renderingInfo);

    VkViewport viewport = {0};
    viewport.width      = (float)solvkstate.swapchainExtent.width;
    viewport.height     = (float)solvkstate.swapchainExtent.height;
    viewport.minDepth   = 0.0f;
    viewport.maxDepth   = 1.0f;
    vkCmdSetViewport(currentCmd, 0, 1, &viewport);

    VkRect2D scissor = {0};
    scissor.extent   = solvkstate.swapchainExtent;
    vkCmdSetScissor(currentCmd, 0, 1, &scissor);
}

void Sol_End_Draw()
{
    VkCommandBuffer currentCmd = solvkstate.commandBuffers[solvkstate.currentFrame];
    vkCmdEndRendering(currentCmd);

    // transition → present
    VkImageMemoryBarrier toPresent        = {0};
    toPresent.sType                       = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    toPresent.oldLayout                   = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    toPresent.newLayout                   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    toPresent.srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
    toPresent.dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
    toPresent.image                       = solvkstate.swapchainImages[solvkstate.currentImageIndex];
    toPresent.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    toPresent.subresourceRange.levelCount = 1;
    toPresent.subresourceRange.layerCount = 1;
    toPresent.srcAccessMask               = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    toPresent.dstAccessMask               = 0;
    vkCmdPipelineBarrier(currentCmd, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &toPresent);

    vkEndCommandBuffer(currentCmd);

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo         = {0};
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = &solvkstate.imageAvailableSemaphores[solvkstate.currentFrame];
    submitInfo.pWaitDstStageMask    = &waitStage;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &currentCmd;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = &solvkstate.renderFinishedSemaphores[solvkstate.currentFrame];

    vkQueueSubmit(solvkstate.graphicsQueue, 1, &submitInfo, solvkstate.inFlightFences[solvkstate.currentFrame]);

    VkPresentInfoKHR presentInfo   = {0};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = &solvkstate.renderFinishedSemaphores[solvkstate.currentFrame];
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &solvkstate.swapchain;
    presentInfo.pImageIndices      = &solvkstate.currentImageIndex;

    vkQueuePresentKHR(solvkstate.graphicsQueue, &presentInfo);

    solvkstate.currentFrame = (solvkstate.currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

int Sol_Pipeline_Build(SolVkState *vkstate, SolPipelineConfig *config, SolPipe *out)
{
    VkDescriptorSetLayout layouts[DESC_COUNT];
    u32                   layoutCount = 0;
    for (int i = 0; i < config->descCount; i++)
    {
        DescriptorId id        = config->descId[i];
        layouts[layoutCount++] = descriptors[id].layout;
    }

    // --- load shader bytecode ---
    SolResource vertRes = Sol_LoadResource(config->vertResource);
    SolResource fragRes = Sol_LoadResource(config->fragResource);

    if (!vertRes.data || !fragRes.data)
        return 1;

    // --- create shader modules ---
    VkShaderModuleCreateInfo vertModuleInfo = {0};
    vertModuleInfo.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertModuleInfo.codeSize                 = vertRes.size;
    vertModuleInfo.pCode                    = (uint32_t *)vertRes.data;

    VkShaderModuleCreateInfo fragModuleInfo = {0};
    fragModuleInfo.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragModuleInfo.codeSize                 = fragRes.size;
    fragModuleInfo.pCode                    = (uint32_t *)fragRes.data;

    VkShaderModule vertModule, fragModule;
    vkCreateShaderModule(vkstate->device, &vertModuleInfo, NULL, &vertModule);
    vkCreateShaderModule(vkstate->device, &fragModuleInfo, NULL, &fragModule);

    // --- shader stages ---
    VkPipelineShaderStageCreateInfo vertStage = {0};
    vertStage.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStage.stage                           = VK_SHADER_STAGE_VERTEX_BIT;
    vertStage.module                          = vertModule;
    vertStage.pName                           = "main";

    VkPipelineShaderStageCreateInfo fragStage = {0};
    fragStage.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStage.stage                           = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStage.module                          = fragModule;
    fragStage.pName                           = "main";

    VkPipelineShaderStageCreateInfo stages[] = {vertStage, fragStage};

    VkVertexInputBindingDescription binding = {
        .binding   = 0,
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };
    VkVertexInputAttributeDescription attrs[5]     = {0};
    uint32_t                          attrCount    = 0;
    uint32_t                          bindingCount = 1;

    if (config->type == VERTEX_TRI)
    {
        binding.stride = sizeof(SolVertex);
        attrs[0]       = (VkVertexInputAttributeDescription){
            .location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(SolVertex, position)};
        attrs[1] = (VkVertexInputAttributeDescription){
            .location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(SolVertex, normal)};
        attrs[2] = (VkVertexInputAttributeDescription){
            .location = 2, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(SolVertex, uv)};
        attrCount = 3;
    }
    else if (config->type == VERTEX_LINE)
    {
        binding.stride = sizeof(SolLineVertex);
        attrs[0]       = (VkVertexInputAttributeDescription){
            .location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(SolLineVertex, pos)};
        attrs[1]  = (VkVertexInputAttributeDescription){.location = 1,
                                                        .binding  = 0,
                                                        .format   = VK_FORMAT_R32G32B32_SFLOAT,
                                                        .offset   = offsetof(SolLineVertex, color)};
        attrCount = 2;
    }
    else if (config->type == VERTEX_SKINNED)
    {
        binding.stride = sizeof(SolVertex);

        attrs[0] = (VkVertexInputAttributeDescription){
            .location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(SolVertex, position)};
        attrs[1] = (VkVertexInputAttributeDescription){
            .location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(SolVertex, normal)};
        attrs[2] = (VkVertexInputAttributeDescription){
            .location = 2, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(SolVertex, uv)};
        attrs[3]  = (VkVertexInputAttributeDescription){.location = 3,
                                                        .binding  = 0,
                                                        .format   = VK_FORMAT_R32G32B32A32_UINT,
                                                        .offset   = offsetof(SolVertex, boneIndices)};
        attrs[4]  = (VkVertexInputAttributeDescription){.location = 4,
                                                        .binding  = 0,
                                                        .format   = VK_FORMAT_R32G32B32A32_SFLOAT,
                                                        .offset   = offsetof(SolVertex, boneWeights)};
        attrCount = 5;
    }
    else
    {
        bindingCount = 0;
    }

    // --- vertex input ---
    VkPipelineVertexInputStateCreateInfo vertexInput = {0};
    vertexInput.sType                                = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount        = bindingCount;
    vertexInput.pVertexBindingDescriptions           = bindingCount ? &binding : NULL;
    vertexInput.vertexAttributeDescriptionCount      = attrCount;
    vertexInput.pVertexAttributeDescriptions         = attrCount ? attrs : NULL;

    // --- input assembly (what shape to draw) ---
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {0};
    inputAssembly.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology                               = config->primitiveTopology;

    // --- viewport and scissor (dynamic so we can resize) ---
    VkPipelineViewportStateCreateInfo viewportState = {0};
    viewportState.sType                             = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount                     = 1;
    viewportState.scissorCount                      = 1;

    // --- rasterizer ---
    VkPipelineRasterizationStateCreateInfo rasterizer = {0};
    rasterizer.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode                            = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode                               = config->cullMode;
    rasterizer.frontFace                              = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.lineWidth                              = 1.0f;

    // --- multisampling (disabled) ---
    VkPipelineMultisampleStateCreateInfo multisampling = {0};
    multisampling.sType                                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples                 = VK_SAMPLE_COUNT_1_BIT;

    // --- color blending (no blending, just write output) ---
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {0};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT; //| VK_COLOR_COMPONENT_A_BIT;
    if (config->blendMode != BLEND_NONE)
    {
        colorBlendAttachment.blendEnable         = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor =
            (config->blendMode == BLEND_ADDITIVE) ? VK_BLEND_FACTOR_ONE : VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;
    }

    VkPipelineColorBlendStateCreateInfo colorBlending = {0};
    colorBlending.sType                               = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount                     = 1;
    colorBlending.pAttachments                        = &colorBlendAttachment;

    // --- dynamic state (viewport and scissor set at draw time) ---
    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicState = {0};
    dynamicState.sType                            = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount                = 2;
    dynamicState.pDynamicStates                   = dynamicStates;

    VkPushConstantRange pushRange = {0};
    pushRange.stageFlags          = config->pushStageFlags;
    pushRange.offset              = 0;
    pushRange.size                = config->pushRangeSize;

    // layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
    pipelineLayoutInfo.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount             = layoutCount;
    pipelineLayoutInfo.pSetLayouts                = layouts;
    pipelineLayoutInfo.pushConstantRangeCount     = config->pushRangeSize > 0 ? 1 : 0;
    pipelineLayoutInfo.pPushConstantRanges        = config->pushRangeSize > 0 ? &pushRange : NULL;
    vkCreatePipelineLayout(vkstate->device, &pipelineLayoutInfo, NULL, &out->layout);

    // --- dynamic rendering info (replaces render pass) ---
    VkPipelineRenderingCreateInfo renderingInfo = {0};
    renderingInfo.sType                         = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount          = 1;
    renderingInfo.pColorAttachmentFormats       = &vkstate->swapchainImageFormat;
    renderingInfo.depthAttachmentFormat         = config->depthTest ? VK_FORMAT_D32_SFLOAT : VK_FORMAT_UNDEFINED;

    // ---- depth stencil ----
    VkPipelineDepthStencilStateCreateInfo depthStencil = {0};
    depthStencil.sType                                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable                       = config->depthTest ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable                      = config->depthWrite ? VK_TRUE : VK_FALSE;
    depthStencil.depthCompareOp                        = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable                 = VK_FALSE;
    depthStencil.stencilTestEnable                     = VK_FALSE;

    // --- create the pipeline ---
    VkGraphicsPipelineCreateInfo pipelineInfo = {0};
    pipelineInfo.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext                        = &renderingInfo;
    pipelineInfo.stageCount                   = 2;
    pipelineInfo.pStages                      = stages;
    pipelineInfo.pVertexInputState            = &vertexInput;
    pipelineInfo.pInputAssemblyState          = &inputAssembly;
    pipelineInfo.pViewportState               = &viewportState;
    pipelineInfo.pRasterizationState          = &rasterizer;
    pipelineInfo.pMultisampleState            = &multisampling;
    pipelineInfo.pColorBlendState             = &colorBlending;
    pipelineInfo.pDynamicState                = &dynamicState;
    pipelineInfo.pDepthStencilState           = &depthStencil;
    pipelineInfo.layout                       = out->layout;

    VkResult result =
        vkCreateGraphicsPipelines(vkstate->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &out->pipeline);
    if (result != VK_SUCCESS)
    {
        Sol_MessageBox("Failed to create graphics pipeline", "Error");
        return 1;
    }

    // shader modules no longer needed after pipeline creation
    vkDestroyShaderModule(vkstate->device, vertModule, NULL);
    vkDestroyShaderModule(vkstate->device, fragModule, NULL);

    return 0;
}

int Sol_Descriptor_Build(SolVkState *vkstate, SolDescriptorConfig *config, SolDescriptor *out)
{
    if (config->kind == DESC_KIND_IMAGE)
    {
        Sol_CreateDescriptorImage(&solvkstate, gpuImages[config->imageId].view, gpuImages[config->imageId].sampler,
                                  VK_SHADER_STAGE_FRAGMENT_BIT, out);
        return 0;
    }
    VkDeviceSize       size       = config->size;
    VkDescriptorType   type       = config->type;
    VkShaderStageFlags stageFlags = config->stageFlags;

    // 1. Descriptor set layout
    VkDescriptorSetLayoutBinding binding = {
        .binding         = 0,
        .descriptorType  = type,
        .descriptorCount = 1,
        .stageFlags      = stageFlags,
    };
    VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings    = &binding,
    };
    vkCreateDescriptorSetLayout(vkstate->device, &layoutInfo, NULL, &out->layout);

    // 2. Descriptor pool
    VkDescriptorPoolSize poolSize = {
        .type            = type,
        .descriptorCount = MAX_FRAMES_IN_FLIGHT,
    };
    VkDescriptorPoolCreateInfo poolInfo = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets       = MAX_FRAMES_IN_FLIGHT,
        .poolSizeCount = 1,
        .pPoolSizes    = &poolSize,
    };
    vkCreateDescriptorPool(vkstate->device, &poolInfo, NULL, &out->pool);

    // 3. Per-frame: buffer + memory + map + descriptor set + write
    VkBufferUsageFlags usage = (type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) ? VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
                                                                           : VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        SolCreateBuffer(vkstate, size, usage,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &out->buffers[i],
                        &out->memory[i]);

        vkMapMemory(vkstate->device, out->memory[i], 0, VK_WHOLE_SIZE, 0, &out->mapped[i]);

        VkDescriptorSetAllocateInfo allocInfo = {
            .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool     = out->pool,
            .descriptorSetCount = 1,
            .pSetLayouts        = &out->layout,
        };
        vkAllocateDescriptorSets(vkstate->device, &allocInfo, &out->sets[i]);

        VkDescriptorBufferInfo bufInfo = {
            .buffer = out->buffers[i],
            .offset = 0,
            .range  = size,
        };
        VkWriteDescriptorSet write = {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = out->sets[i],
            .dstBinding      = 0,
            .descriptorType  = type,
            .descriptorCount = 1,
            .pBufferInfo     = &bufInfo,
        };
        vkUpdateDescriptorSets(vkstate->device, 1, &write, 0, NULL);
    }

    return 0;
}

int Sol_UploadModel(SolModel *model, SolModelId modelId)
{
    // 1. Pre-cleanup to prevent memory leaks if overwriting an existing ID
    if (gpuModels[modelId].meshes != NULL)
        free(gpuModels[modelId].meshes);

    // if (!model || model->mesh_count < 1)
    //     return;
    SolGpuModel gpuModel = {0};
    gpuModel.mesh_count  = model->mesh_count;
    gpuModel.meshes      = malloc(sizeof(SolGpuMesh) * model->mesh_count);

    // 2. Calculate total memory needed for a single staging allocation
    VkDeviceSize totalSize = 0;
    totalSize += sizeof(SolVertex) * model->vertex_count;
    totalSize += sizeof(uint32_t) * model->indice_count;

    // 3. Create a single staging buffer for all data
    VkBuffer       stagingBuffer;
    VkDeviceMemory stagingMemory;
    SolCreateBuffer(&solvkstate, totalSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer,
                    &stagingMemory);

    // 4. Map once and copy everything
    void *data;
    vkMapMemory(solvkstate.device, stagingMemory, 0, totalSize, 0, &data);

    VkDeviceSize verticesSize = sizeof(SolVertex) * model->vertex_count;
    VkDeviceSize indicesSize  = sizeof(uint32_t) * model->indice_count;
    memcpy((uint8_t *)data, model->vertices, verticesSize);
    memcpy((uint8_t *)data + verticesSize, model->indices, indicesSize);

    vkUnmapMemory(solvkstate.device, stagingMemory);

    // 5. Setup single command buffer for batch transfer
    VkCommandBufferAllocateInfo allocInfo = {.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                                             .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                             .commandPool        = solvkstate.commandPool,
                                             .commandBufferCount = 1};
    VkCommandBuffer             copyCmd;
    vkAllocateCommandBuffers(solvkstate.device, &allocInfo, &copyCmd);

    VkCommandBufferBeginInfo beginInfo = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                                          .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
    vkBeginCommandBuffer(copyCmd, &beginInfo);

    // 6. Create GPU buffers and record copy commands
    for (uint32_t m = 0; m < model->mesh_count; m++)
    {
        SolMesh    *src = &model->meshes[m];
        SolGpuMesh *dst = &gpuModel.meshes[m];
        dst->indexCount = src->indexCount;
        dst->material   = src->material;

        VkDeviceSize vSize = sizeof(SolVertex) * src->vertexCount;
        VkDeviceSize iSize = sizeof(uint32_t) * src->indexCount;

        VkDeviceSize vSrcOffset = sizeof(SolVertex) * src->vertexOffset;
        VkDeviceSize iSrcOffset = verticesSize + sizeof(uint32_t) * src->indexOffset;
        // Create Device Local Buffers
        SolCreateBuffer(&solvkstate, vSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &dst->vertexBuffer, &dst->vertexMemory);
        SolCreateBuffer(&solvkstate, iSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &dst->indexBuffer, &dst->indexMemory);

        // Record Copies
        VkBufferCopy vCopy = {.srcOffset = vSrcOffset, .dstOffset = 0, .size = vSize};
        vkCmdCopyBuffer(copyCmd, stagingBuffer, dst->vertexBuffer, 1, &vCopy);

        VkBufferCopy iCopy = {.srcOffset = iSrcOffset, .dstOffset = 0, .size = iSize};
        vkCmdCopyBuffer(copyCmd, stagingBuffer, dst->indexBuffer, 1, &iCopy);
    }

    vkEndCommandBuffer(copyCmd);

    // TODO POOL AND UPLOAD AT ONCE

    // 7. Submit and wait ONCE
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1, .pCommandBuffers = &copyCmd};
    vkQueueSubmit(solvkstate.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(solvkstate.graphicsQueue);

    // 8. Clean up temporary staging resources
    vkFreeCommandBuffers(solvkstate.device, solvkstate.commandPool, 1, &copyCmd);
    vkDestroyBuffer(solvkstate.device, stagingBuffer, NULL);
    vkFreeMemory(solvkstate.device, stagingMemory, NULL);

    gpuModels[modelId] = gpuModel;
    printf("SolVk: Uploaded Model %d (%d meshes)\n", modelId, gpuModel.mesh_count);

    return 0;
}

int Sol_UploadImage(SolTexture *image, SolTextureId id)
{
    const void *pixels = image->pixels;
    u32         width  = image->width;
    u32         height = image->height;
    int         format = 37;

    SolGpuImage *out     = &gpuImages[id];
    SolVkState  *vkstate = &solvkstate;

    VkDeviceSize imageSize = width * height * 4; // assumes 4 bytes per pixel

    // 1. Staging buffer
    VkBuffer       staging;
    VkDeviceMemory stagingMem;
    if (SolCreateBuffer(&solvkstate, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging,
                        &stagingMem) != 0)
        return 1;

    void *mapped;
    vkMapMemory(solvkstate.device, stagingMem, 0, imageSize, 0, &mapped);
    memcpy(mapped, pixels, imageSize);
    vkUnmapMemory(solvkstate.device, stagingMem);

    // 2. Create image
    VkImageCreateInfo imageInfo = {
        .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType     = VK_IMAGE_TYPE_2D,
        .format        = format,
        .extent        = {width, height, 1},
        .mipLevels     = 1,
        .arrayLayers   = 1,
        .samples       = VK_SAMPLE_COUNT_1_BIT,
        .tiling        = VK_IMAGE_TILING_OPTIMAL,
        .usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    vkCreateImage(vkstate->device, &imageInfo, NULL, &out->image);

    // 3. Allocate and bind memory
    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(vkstate->device, out->image, &memReqs);

    uint32_t memIndex;
    if (SolFindMemoryType(vkstate->physicalDevice, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                          &memIndex) != 0)
    {
        vkDestroyImage(vkstate->device, out->image, NULL);
        vkDestroyBuffer(vkstate->device, staging, NULL);
        vkFreeMemory(vkstate->device, stagingMem, NULL);
        return 1;
    }

    VkMemoryAllocateInfo allocInfo = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = memReqs.size,
        .memoryTypeIndex = memIndex,
    };
    vkAllocateMemory(vkstate->device, &allocInfo, NULL, &out->memory);
    vkBindImageMemory(vkstate->device, out->image, out->memory, 0);

    // 4. Transfer via one-time command buffer
    VkCommandBufferAllocateInfo cmdAlloc = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = vkstate->commandPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(vkstate->device, &cmdAlloc, &cmd);

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vkBeginCommandBuffer(cmd, &beginInfo);

    // Transition: undefined → transfer dst
    VkImageMemoryBarrier toTransfer = {
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image               = out->image,
        .subresourceRange    = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
        .srcAccessMask       = 0,
        .dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT,
    };
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1,
                         &toTransfer);

    // Copy buffer → image
    VkBufferImageCopy region = {
        .imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
        .imageExtent      = {width, height, 1},
    };
    vkCmdCopyBufferToImage(cmd, staging, out->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // Transition: transfer dst → shader read
    VkImageMemoryBarrier toRead = {
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image               = out->image,
        .subresourceRange    = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
        .srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask       = VK_ACCESS_SHADER_READ_BIT,
    };
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0,
                         NULL, 1, &toRead);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo = {
        .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers    = &cmd,
    };
    vkQueueSubmit(vkstate->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(vkstate->graphicsQueue);

    // 5. Cleanup staging
    vkFreeCommandBuffers(vkstate->device, vkstate->commandPool, 1, &cmd);
    vkDestroyBuffer(vkstate->device, staging, NULL);
    vkFreeMemory(vkstate->device, stagingMem, NULL);

    // 6. Image view
    VkImageViewCreateInfo viewInfo = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image            = out->image,
        .viewType         = VK_IMAGE_VIEW_TYPE_2D,
        .format           = format,
        .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
    };
    vkCreateImageView(vkstate->device, &viewInfo, NULL, &out->view);

    // 7. Sampler
    VkSamplerCreateInfo samplerInfo = {
        .sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter    = VK_FILTER_LINEAR,
        .minFilter    = VK_FILTER_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    };
    vkCreateSampler(vkstate->device, &samplerInfo, NULL, &out->sampler);

    return 0;
}
