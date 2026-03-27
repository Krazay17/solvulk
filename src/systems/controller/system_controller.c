#include "sol_core.h"

void Sol_System_Controller_Tick(World *world, double dt, double time)
{
    SolMouse mouse = SolInput_GetMouse();
    int required = HAS_CONTROLLER;
    for (int i = 0; i < world->activeCount; ++i)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) == required)
        {
            CompController *controller = &world->controllers[id];
            controller->yaw += mouse.dx;
            controller->pitch += mouse.dy;

            if (SolInput_KeyDown(SOL_KEY_W))
                controller->actionState |= ACTION_FWD;
            else
                controller->actionState &= ~ACTION_FWD;

            if (SolInput_KeyDown(SOL_KEY_S))
                controller->actionState |= ACTION_BWD;
            else
                controller->actionState &= ~ACTION_BWD;

            if (SolInput_KeyDown(SOL_KEY_A))
                controller->actionState |= ACTION_LEFT;
            else
                controller->actionState &= ~ACTION_LEFT;

            if (SolInput_KeyDown(SOL_KEY_D))
                controller->actionState |= ACTION_RIGHT;
            else
                controller->actionState &= ~ACTION_RIGHT;
        }
    }
}