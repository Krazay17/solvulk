#include "render.h"
#include "sol_core.h"

float     solAspectRatio = 16.0f / 9.0f;
SolCamera renderCam      = {
    .fov      = 60.0f,
    .nearClip = 0.2f,
    .farClip  = 1000.0f,
};

void Render_Push_Model(SolModelDraw model)
{
    // printf("Push model");
    // Render_Models(model.modelId);
}

void Render_Init(void *hwnd, void *hInstance)
{
    int vulkInit = Sol_Init_Vulkan(hwnd, hInstance);
    printf("Vulkan Init code: %d\n", vulkInit);

    SolBank *bank = Sol_Bank_Get();
    for (int i = 0; i < SOL_MODEL_COUNT; i++)
        Sol_UploadModel(&bank->models[i], i);
    for (int i = 0; i < SOL_IMAGE_COUNT; i++)
        Sol_UploadImage(bank->images[i].pixels, 224, 224, 37, i);

    Sol_Init_Vulkan_Resources();
    // solAspectRatio = Sol_GetState()->windowWidth / Sol_GetState()->windowHeight;
}

void Render_Camera_Update(vec3 pos, vec3 target)
{
    // 1. Update vectors
    glm_vec3_copy(pos, renderCam.position);
    glm_vec3_copy(target, renderCam.target);

    // 2. View Matrix
    glm_lookat(renderCam.position, renderCam.target, (vec3){0.0f, 1.0f, 0.0f}, renderCam.view);

    // 3. Projection Matrix
    glm_perspective(glm_rad(renderCam.fov), solAspectRatio, renderCam.nearClip, renderCam.farClip, renderCam.proj);
    renderCam.proj[1][1] *= -1;

    // 4. View Projection
    glm_mat4_mul(renderCam.proj, renderCam.view, renderCam.viewProj);
}

SolCamera *Sol_GetCamera()
{
    return &renderCam;
}

void Sol_Render_Resize(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0)
        return;
    solAspectRatio = (float)width / (float)height;
    Vk_SetOrtho(width, height);
    Sol_GetState()->windowWidth  = (float)width;
    Sol_GetState()->windowHeight = (float)height;
    Remake_Swapchain(width, height);
}

// void Flush_Models(void)
// {
//     if (modelSubmission.count == 0)
//         return;

//     // Count per handle
//     uint32_t counts[SOL_MODEL_COUNT] = {0};
//     for (int i = 0; i < modelSubmission.count; i++)
//         counts[modelSubmission.handles[i]]++;

//     // Prefix sum
//     uint32_t offsets[SOL_MODEL_COUNT] = {0};
//     for (int i = 1; i < SOL_MODEL_COUNT; i++)
//         offsets[i] = offsets[i - 1] + counts[i - 1];

//     // Write sorted into SSBO
//     ModelSSBO *gpu = descriptors[DESC_MODEL_SSBO].mapped[solvkstate.currentFrame];
//     FlagsSSBO *f   = descriptors[DESC_FLAGS_SSBO].mapped[solvkstate.currentFrame];

//     uint32_t cursors[SOL_MODEL_COUNT];
//     memcpy(cursors, offsets, sizeof(offsets));

//     for (int i = 0; i < modelSubmission.count; i++)
//     {
//         SolModelId h    = modelSubmission.handles[i];
//         gpu[cursors[h]] = modelSubmission.instances[i];
//         f[cursors[h]]   = modelSubmission.flags[i];
//         cursors[h]++;
//     }

//     for (int h = 0; h < SOL_MODEL_COUNT; h++)
//     {
//         if (counts[h] > 0)
//         {
//             Render_Model(h, counts[h], offsets[h]);
//         }
//     }

//     model_que_offset += modelSubmission.count;
//     modelSubmission.count = 0;
// }