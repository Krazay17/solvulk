#include <cglm/cglm.h>
#include <cglm/struct.h>

#include "sol_core.h"

SolVkState solvkstate = {0};

float solAspectRatio;
SolCamera renderCam = {
    .fov = 60.0f,
    .nearClip = 0.1f,
    .farClip = 5000.0f,
};
mat4 ortho2d;

static SolDescriptorBuffer sceneUBO;

static SolPipeModel pipeModel;
static SolPipeRect pipeRect;
static SolPipeText pipeText;
static SolPipeRay pipeRay;
static SolGpuModel gpuModels[MAX_GPU_MODELS];

static VkPipeline currentBoundPipeline = VK_NULL_HANDLE;

int Sol_Init_Vulkan(void *hwnd, void *hInstance)
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

    if (Sol_Pipeline_BuildAllDefault(&solvkstate) != 0)
        return 9;
    if (SolVkSyncObjects(&solvkstate) != 0)
        return 10;

    uint32_t width = solvkstate.swapchainExtent.width;
    uint32_t height = solvkstate.swapchainExtent.height;
    solAspectRatio = (float)width / (float)height;
    Sol_SetOrtho(width, height);

    return 0;
}

VkCommandBuffer Command_Buffer_Get()
{
    return solvkstate.commandBuffers[solvkstate.currentFrame];
}

void Bind_Pipeline(VkCommandBuffer cmd, VkPipeline pipeline)
{
    if (pipeline == currentBoundPipeline)
        return;
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    currentBoundPipeline = pipeline;
}

static float SolColorF(uint8_t c) { return c / 255.0f; }

void Sol_SetOrtho(uint32_t width, uint32_t height)
{
    glm_ortho(0.0f, width, 0.0f, height, -1.0f, 1.0f, ortho2d);
}

void Sol_Render_Resize()
{
    uint32_t width = solvkstate.swapchainExtent.width;
    uint32_t height = solvkstate.swapchainExtent.height;
    if (width == 0 || height == 0)
        return;
    vkDeviceWaitIdle(solvkstate.device);

    // destroy old
    vkDestroyImageView(solvkstate.device, solvkstate.depthImageView, NULL);
    vkDestroyImage(solvkstate.device, solvkstate.depthImage, NULL);
    vkFreeMemory(solvkstate.device, solvkstate.depthMemory, NULL);

    for (uint32_t i = 0; i < solvkstate.swapchainImageCount; i++)
        vkDestroyImageView(solvkstate.device, solvkstate.swapchainImageViews[i], NULL);

    vkDestroySwapchainKHR(solvkstate.device, solvkstate.swapchain, NULL);

    // recreate
    SolVkSwapchain(&solvkstate);
    SolVkImageViews(&solvkstate);
    SolVkDepthResources(&solvkstate);
    Sol_SetOrtho(width, height);
    Sol_GetState()->windowWidth = (float)width;
    Sol_GetState()->windowHeight = (float)height;
    solAspectRatio = (float)width / (float)height;
}

SolCamera *Sol_GetCamera()
{
    return &renderCam;
}

void Sol_Camera_Update(vec3 pos, vec3 target)
{
    // 1. Update vectors
    glm_vec3_copy(pos, renderCam.position);
    glm_vec3_copy(target, renderCam.target);

    // 2. View Matrix
    vec3 up = {0.0f, 1.0f, 0.0f};
    glm_lookat(renderCam.position, renderCam.target, up, renderCam.view);

    // 3. Projection Matrix
    glm_perspective(glm_rad(renderCam.fov), solAspectRatio, renderCam.nearClip, renderCam.farClip, renderCam.proj);
    renderCam.proj[1][1] *= -1;
}

int Sol_Pipeline_BuildAllDefault(SolVkState *vkstate)
{
    // ─── Text Pipeline ──────────────────────────────────────────
    SolResource metrics = Sol_LoadResource("ID_FONT_METRICS");
    if (metrics.data)
        Sol_ParseFontMetrics((const char *)metrics.data, 224.0f, 224.0f, pipeText.glyphs);

    SolResource fontRes = Sol_LoadResource("ID_FONT_ATLAS");
    if (!fontRes.data)
        return 1;

    if (Sol_UploadImage(vkstate, fontRes.data, 224, 224,
                        VK_FORMAT_R8G8B8A8_UNORM, &pipeText.fontAtlas) != 0)
        return 1;

    if (Sol_CreateDescriptorImage(vkstate, pipeText.fontAtlas.view, pipeText.fontAtlas.sampler,
                                  VK_SHADER_STAGE_FRAGMENT_BIT, &pipeText.fontDesc) != 0)
        return 1;

    SolPipelineConfig textConfig = {
        .vertResource = "ID_SHADER_TEXTV",
        .fragResource = "ID_SHADER_TEXTF",
        .depthTest = 0,
        .alphaBlend = 0,
        .cullMode = VK_CULL_MODE_NONE,
        .pushRangeSize = sizeof(SolTextPush),
        .pushStageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    };

    if (Sol_BuildPipeline(vkstate, &textConfig, &pipeText.fontDesc.layout, 1, &pipeText.pipe) != 0)
        return 1;

    // ─── 3D Mesh Pipeline ───────────────────────────────────────
    if (Sol_CreateDescriptorBuffer(vkstate, sizeof(SceneUBO),
                                   VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                   &sceneUBO) != 0)
        return 1;

    if (Sol_CreateDescriptorBuffer(vkstate, sizeof(ModelSSBO) * MAX_MODEL_INSTANCES,
                                   VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                   VK_SHADER_STAGE_VERTEX_BIT,
                                   &pipeModel.modelSSBO) != 0)
        return 1;

    VkDescriptorSetLayout meshLayouts[] = {
        sceneUBO.layout,
        pipeModel.modelSSBO.layout,
    };

    SolPipelineConfig meshConfig = {
        .vertResource = "ID_SHADER_MODELV",
        .fragResource = "ID_SHADER_MODELF",
        .depthTest = 1,
        .alphaBlend = 1,
        .cullMode = VK_CULL_MODE_NONE,
        .pushRangeSize = sizeof(SolMaterial),
        .pushStageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    };

    if (Sol_BuildPipeline(vkstate, &meshConfig, meshLayouts, 2, &pipeModel.pipe) != 0)
        return 1;

    // ─── 2D Rect Pipeline ───────────────────────────────────────
    SolPipelineConfig rectConfig = {
        .vertResource = "ID_SHADER_RECTV",
        .fragResource = "ID_SHADER_RECTF",
        .depthTest = 0,
        .alphaBlend = 1,
        .cullMode = VK_CULL_MODE_NONE,
        .pushRangeSize = sizeof(float) * 32,
        .pushStageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    };

    if (Sol_BuildPipeline(vkstate, &rectConfig, NULL, 0, &pipeRect.pipe) != 0)
        return 1;

    SolPipelineConfig lineConfig = {
        .vertResource = "ID_SHADER_LINEV",
        .fragResource = "ID_SHADER_LINEF",
        .depthTest = 0,
        .alphaBlend = 1,
        .cullMode = VK_CULL_MODE_NONE,
        .pushRangeSize = sizeof(float) * 32,
        .pushStageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    };

    VkDescriptorSetLayout lineLayouts[] = {
        sceneUBO.layout,
    };

    if (Sol_BuildPipeline(vkstate, &lineConfig, lineLayouts, 1, &pipeRay.pipe) != 0)
        return 1;

    return 0;
}

void Sol_Draw_Rectangle(SolRect rect, SolColor color, float thickness)
{
    VkCommandBuffer cmd = Command_Buffer_Get();
    Bind_Pipeline(cmd, pipeRect.pipe.pipeline);

    struct
    {
        mat4 o;
        vec4 rec;
        vec4 c;
        vec4 extras;
    } push = {
        .rec = {rect.x, rect.y, rect.w, rect.h},
        .c = {SolColorF(color.r), SolColorF(color.g), SolColorF(color.b), SolColorF(color.a)},
        .extras = {thickness, 0, 0, 0},
    };
    memcpy(push.o, ortho2d, sizeof(mat4));

    vkCmdPushConstants(cmd, pipeRect.pipe.layout,
                       VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push), &push);

    vkCmdDraw(cmd, 6, 1, 0, 0);
}

void Sol_Begin_3D()
{
    SceneUBO *ubo = sceneUBO.mapped[solvkstate.currentFrame];
    glm_mat4_mul(renderCam.proj, renderCam.view, ubo->viewProjection);
    memcpy(ubo->view, renderCam.view, sizeof(mat4));
    memcpy(ubo->proj, renderCam.proj, sizeof(mat4));
    memcpy(ubo->cameraPos, renderCam.position, sizeof(vec3));
}

void Sol_Draw_Model_Instanced(SolModelId handle, uint32_t instanceCount, uint32_t firstInstance)
{
    VkCommandBuffer cmd = Command_Buffer_Get();
    Bind_Pipeline(cmd, pipeModel.pipe.pipeline);

    VkDescriptorSet sets[2] = {
        sceneUBO.sets[solvkstate.currentFrame],
        pipeModel.modelSSBO.sets[solvkstate.currentFrame]};
    // Bind the SSBO for the CURRENT frame
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipeModel.pipe.layout, 0, 2,
                            sets, 0, NULL);

    SolGpuModel *model = &gpuModels[handle];
    for (uint32_t m = 0; m < model->mesh_count; m++)
    {
        vkCmdPushConstants(cmd, pipeModel.pipe.layout, VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, sizeof(SolMaterial), &model->meshes[m].material);
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmd, 0, 1, &model->meshes[m].vertexBuffer, offsets);
        vkCmdBindIndexBuffer(cmd, model->meshes[m].indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmd, model->meshes[m].indexCount, instanceCount, 0, 0, firstInstance);
    }
}

void Sol_Draw_Line(SolLine *lines, int count)
{
    VkCommandBuffer cmd = Command_Buffer_Get();
    Bind_Pipeline(cmd, pipeRay.pipe.pipeline);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipeRay.pipe.layout, 0, 1,
                            &sceneUBO.sets[solvkstate.currentFrame], 0, NULL);

    vkCmdDraw(cmd, count * 2, 1, 0, 0);
}

void Sol_Draw_Text(const char *str, float x, float y, float size, SolColor color)
{
    // atlas layout constants — match your exported atlas
    const int COLS = 16;
    const int ROWS = 6;
    const float CELL_W = 1.0f / COLS;
    const float CELL_H = 1.0f / ROWS;
    const float CHAR_W = size * 0.6f; // aspect ratio of one glyph
    const float CHAR_H = size;

    VkCommandBuffer cmd = Command_Buffer_Get();
    Bind_Pipeline(cmd, pipeText.pipe.pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipeText.pipe.layout, 0, 1,
                            &pipeText.fontDesc.set, 0, NULL);

    float cursorX = x;
    for (const char *c = str; *c; c++)
    {
        int ascii = (int)(*c) - 32;
        if (ascii < 0 || ascii >= COLS * ROWS)
        {
            cursorX += CHAR_W;
            continue;
        }

        int col = ascii % COLS;
        int row = ascii / COLS;
        float baseSize = size / 32.0f;
        SolGlyph *g = &pipeText.glyphs[(int)*c];

        SolTextPush push;
        memcpy(push.ortho, ortho2d, sizeof(mat4));

        float pad = 0.2f * baseSize * (224.0f / 32.0f);
        push.x = cursorX + g->xoffset * size - pad;
        push.y = y - g->ytop * size - pad;
        push.w = g->uw * 224.0f * baseSize + pad * 2.0f;
        push.h = g->vh * 224.0f * baseSize + pad * 2.0f;
        push.r = SolColorF(color.r);
        push.g = SolColorF(color.g);
        push.b = SolColorF(color.b);
        push.a = SolColorF(color.a);
        push.u = g->u;
        push.v = 1.0f - g->v - g->vh; // flip for bottom-origin atlas
        push.uw = g->uw;
        push.vh = g->vh;
        cursorX += g->yadvance * size; // only advance once, remove the CHAR_W line

        vkCmdPushConstants(cmd, pipeText.pipe.layout,
                           VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(SolTextPush), &push);
        vkCmdDraw(cmd, 6, 1, 0, 0);
    }
}

float Sol_MeasureText(const char *str, float size)
{
    float width = 0.0f;
    for (const char *c = str; *c; c++)
    {
        int id = (int)*c;
        if (id < 0 || id >= 128)
            continue;
        width += pipeText.glyphs[id].yadvance * size;
    }
    return width;
}

void Sol_Begin_Draw()
{
    // solvkstate.currentBoundPipeline = -1;
    vkWaitForFences(solvkstate.device, 1, &solvkstate.inFlightFences[solvkstate.currentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(solvkstate.device, 1, &solvkstate.inFlightFences[solvkstate.currentFrame]);

    vkAcquireNextImageKHR(solvkstate.device, solvkstate.swapchain, UINT64_MAX,
                          solvkstate.imageAvailableSemaphores[solvkstate.currentFrame],
                          VK_NULL_HANDLE, &solvkstate.currentImageIndex);

    VkCommandBuffer currentCmd = solvkstate.commandBuffers[solvkstate.currentFrame];
    vkResetCommandBuffer(currentCmd, 0);

    VkCommandBufferBeginInfo beginInfo = {0};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(currentCmd, &beginInfo);

    VkImageMemoryBarrier depthBarrier = {0};
    depthBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    depthBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depthBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    depthBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    depthBarrier.image = solvkstate.depthImage;
    depthBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    depthBarrier.subresourceRange.levelCount = 1;
    depthBarrier.subresourceRange.layerCount = 1;
    depthBarrier.srcAccessMask = 0;
    depthBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    vkCmdPipelineBarrier(currentCmd,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                         0, 0, NULL, 0, NULL, 1, &depthBarrier);

    // clear depth
    VkClearValue depthClear = {0};
    depthClear.depthStencil.depth = 1.0f;

    VkRenderingAttachmentInfo depthAttachment = {0};
    depthAttachment.clearValue = depthClear;
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.imageView = solvkstate.depthImageView;
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    // transition → color attachment
    VkImageMemoryBarrier toRender = {0};
    toRender.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    toRender.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    toRender.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    toRender.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toRender.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toRender.image = solvkstate.swapchainImages[solvkstate.currentImageIndex];
    toRender.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    toRender.subresourceRange.levelCount = 1;
    toRender.subresourceRange.layerCount = 1;
    toRender.srcAccessMask = 0;
    toRender.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    vkCmdPipelineBarrier(currentCmd,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         0, 0, NULL, 0, NULL, 1, &toRender);

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};

    VkRenderingAttachmentInfo colorAttachment = {0};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = solvkstate.swapchainImageViews[solvkstate.currentImageIndex];
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue = clearColor;

    VkRenderingInfo renderingInfo = {0};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.extent = solvkstate.swapchainExtent;
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;
    renderingInfo.pDepthAttachment = &depthAttachment;

    vkCmdBeginRendering(currentCmd, &renderingInfo);

    VkViewport viewport = {0};
    viewport.width = (float)solvkstate.swapchainExtent.width;
    viewport.height = (float)solvkstate.swapchainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(currentCmd, 0, 1, &viewport);

    VkRect2D scissor = {0};
    scissor.extent = solvkstate.swapchainExtent;
    vkCmdSetScissor(currentCmd, 0, 1, &scissor);
}

void Sol_End_Draw()
{
    VkCommandBuffer currentCmd = solvkstate.commandBuffers[solvkstate.currentFrame];
    vkCmdEndRendering(currentCmd);

    // transition → present
    VkImageMemoryBarrier toPresent = {0};
    toPresent.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    toPresent.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    toPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    toPresent.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toPresent.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toPresent.image = solvkstate.swapchainImages[solvkstate.currentImageIndex];
    toPresent.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    toPresent.subresourceRange.levelCount = 1;
    toPresent.subresourceRange.layerCount = 1;
    toPresent.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    toPresent.dstAccessMask = 0;
    vkCmdPipelineBarrier(currentCmd,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         0, 0, NULL, 0, NULL, 1, &toPresent);

    vkEndCommandBuffer(currentCmd);

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo = {0};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &solvkstate.imageAvailableSemaphores[solvkstate.currentFrame];
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &currentCmd;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &solvkstate.renderFinishedSemaphores[solvkstate.currentFrame];

    vkQueueSubmit(solvkstate.graphicsQueue, 1, &submitInfo, solvkstate.inFlightFences[solvkstate.currentFrame]);

    VkPresentInfoKHR presentInfo = {0};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &solvkstate.renderFinishedSemaphores[solvkstate.currentFrame];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &solvkstate.swapchain;
    presentInfo.pImageIndices = &solvkstate.currentImageIndex;

    vkQueuePresentKHR(solvkstate.graphicsQueue, &presentInfo);

    solvkstate.currentFrame = (solvkstate.currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Sol_UploadModel(SolModel *model, SolModelId modelId)
{
    // 1. Pre-cleanup to prevent memory leaks if overwriting an existing ID
    if (gpuModels[modelId].meshes != NULL)
        free(gpuModels[modelId].meshes);

    // if (!model || model->mesh_count < 1)
    //     return;
    SolGpuModel gpuModel = {0};
    gpuModel.mesh_count = model->mesh_count;
    gpuModel.meshes = malloc(sizeof(SolGpuMesh) * model->mesh_count);

    // 2. Calculate total memory needed for a single staging allocation
    VkDeviceSize totalSize = 0;
    totalSize += sizeof(SolVertex) * model->vertex_count;
    totalSize += sizeof(uint32_t) * model->indice_count;

    // 3. Create a single staging buffer for all data
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;
    SolCreateBuffer(&solvkstate, totalSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    &stagingBuffer, &stagingMemory);

    // 4. Map once and copy everything
    void *data;
    vkMapMemory(solvkstate.device, stagingMemory, 0, totalSize, 0, &data);

    VkDeviceSize verticesSize = sizeof(SolVertex) * model->vertex_count;
    VkDeviceSize indicesSize = sizeof(uint32_t) * model->indice_count;
    memcpy((uint8_t *)data, model->vertices, verticesSize);
    memcpy((uint8_t *)data + verticesSize, model->indices, indicesSize);

    vkUnmapMemory(solvkstate.device, stagingMemory);

    // 5. Setup single command buffer for batch transfer
    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool = solvkstate.commandPool,
        .commandBufferCount = 1};
    VkCommandBuffer copyCmd;
    vkAllocateCommandBuffers(solvkstate.device, &allocInfo, &copyCmd);

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
    vkBeginCommandBuffer(copyCmd, &beginInfo);

    // 6. Create GPU buffers and record copy commands
    for (uint32_t m = 0; m < model->mesh_count; m++)
    {
        SolMesh *src = &model->meshes[m];
        SolGpuMesh *dst = &gpuModel.meshes[m];
        dst->indexCount = src->indexCount;
        dst->material = src->material;

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

    // 7. Submit and wait ONCE
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &copyCmd};
    vkQueueSubmit(solvkstate.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(solvkstate.graphicsQueue);

    // 8. Clean up temporary staging resources
    vkFreeCommandBuffers(solvkstate.device, solvkstate.commandPool, 1, &copyCmd);
    vkDestroyBuffer(solvkstate.device, stagingBuffer, NULL);
    vkFreeMemory(solvkstate.device, stagingMemory, NULL);

    gpuModels[modelId] = gpuModel;
    printf("SolVk: Uploaded Model %d (%d meshes)\n", modelId, gpuModel.mesh_count);
}

void *Sol_ModelBuffer_Get()
{
    return pipeModel.modelSSBO.mapped[solvkstate.currentFrame];
}