#include "sol_core.h"

#include "render.h"
#include "model/model.h"

float solAspectRatio = 16.0f / 9.0f;

void Render_Camera_Update(SolCamera *cam)
{
    // View Matrix
    glm_lookat(cam->position, cam->target, (vec3){0.0f, 1.0f, 0.0f}, cam->view);

    // Projection Matrix
    glm_perspective(glm_rad(cam->fov), solAspectRatio, cam->nearClip, cam->farClip, cam->proj);
    cam->proj[1][1] *= -1;

    // View Projection
    glm_mat4_mul(cam->proj, cam->view, cam->viewProj);

    SceneUBO *ubo = Sol_GetDescriptorMapping(DESC_SCENE_UBO);
    glm_mat4_copy(cam->view, ubo->view);
    glm_mat4_copy(cam->proj, ubo->proj);
    glm_mat4_copy(cam->viewProj, ubo->viewProjection);
    vec4 pos = {cam->position[0], cam->position[1], cam->position[2], 1.0f};
    glm_vec4_copy(pos, ubo->cameraPos);
    ubo->sun[0] = 0.0f;
    ubo->sun[1] = 1.0f;
    ubo->sun[2] = 0.4f;
    ubo->sun[3] = 0.2f;
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