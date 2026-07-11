/*
 * File: s_pickup.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * Pickup!
 */
#include "s_interact.h"
#include "sol_math.h"
#include "world.h"
#include "input.h"
#include "xform/s_xform.h"
#include "physx/s_body.h"

float stiffness = 5.0f;

void Sol_Pickup_Init(World *world)
{
    WAddStep(world) = Sol_Pickup_Step;
}

static int      step_required = BITC(HAS_INTERACT);
void Sol_Pickup_Step(World *world, double dt, double time)
{
    SolMouse mouse    = Sol_Input_GetMouse();
    for (int i = 0; i < world->activeCount; i++)
    {
        u32 id = world->activeEntities[i];
        if (!WHas(world, id, step_required) || !(world->flags[id].flags & EFLAG_PICKUPABLE))
            continue;

        CompFlags *flags = &world->flags[id];

        if (!Sol_Input_GetMouse().buttons[SOL_MOUSE_LEFT])
        {
            world->flags[id].flags &= ~EFLAG_PICKEDUP;
        }

        if (world->flags[id].flags & EFLAG_PICKEDUP)
        {
            vec3s targetPos = Sol_ScreenRaycast(world, mouse.x, mouse.y, (SolRay){.dist = 20.0f}).pos;
            vec3s vel       = vecSca(vecSub(targetPos, Sol_Xform_GetPos(world, id)), stiffness);
            Sol_Physx_SetVel(world, id, vel);

            continue;
        }

        if (Sol_Interact_GetState(world, id) & INTERACT_PRESSED)
        {
            world->flags[id].flags |= EFLAG_PICKEDUP;
        }
    }
}