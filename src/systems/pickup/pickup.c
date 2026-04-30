#include "sol_core.h"

u32   pickupId  = -1;
float stiffness = 5.0f;

void Pickup_Step(World *world, double dt, double time)
{
    SolMouse mouse    = Sol_Input_GetMouse();
    u32      required = HAS_INTERACT;
    for (int i = 0; i < world->activeCount; i++)
    {
        u32 id = world->activeEntities[i];
        if ((world->masks[id] & required) != required || !(world->flags[id].flags & EFLAG_PICKUPABLE))
            continue;
            
        CompFlags    *flags    = &world->flags[id];
        CompXform    *xform    = &world->xforms[id];
        CompBody     *body     = &world->bodies[id];
        CompInteract *interact = &world->interacts[id];

        if (pickupId == id)
        {
            world->flags[id].flags |= EFLAG_PICKEDUP;
            vec3s targetPos = Sol_ScreenRaycast(world, mouse.x, mouse.y, (SolRay){.dist = 20.0f}).pos;
            body->vel       = vecSca(vecSub(targetPos, xform->pos), stiffness);
            return;
        }

        if (interact->states & INTERACT_PRESSED)
        {
            pickupId = id;
        }
        if (!Sol_Input_GetMouse().buttons[SOL_MOUSE_LEFT])
        {
            pickupId = -1;
            world->flags[id].flags &= ~EFLAG_PICKEDUP;
        }
    }
}