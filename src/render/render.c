#include "sol_core.h"

#include "render.h"

void Sol_Render_Resize(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0)
        return;
    Sol_Render_SetOrtho(width, height);
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