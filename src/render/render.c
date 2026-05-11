#include "sol_core.h"

void Sol_Render_Resize(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0)
        return;
    Sol_Render_SetOrtho(width, height);
    Remake_Swapchain(width, height);
}

void Flush_Queue(void)
{
    Flush_Models();
    Flush_Models_Skinned();
    Flush_Billboards();
}