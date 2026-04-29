#include "sol_core.h"

u32 pickupId = -1;
float stiffness = 5.0f;

CompPickup *Sol_Pickup_Add(World *world, int id, CompPickup init)
{
    CompPickup pickup  = {0};
    world->pickups[id] = pickup;
    world->masks[id] |= HAS_PICKUP;
    return &world->pickups[id];
}

void Pickup_Step(World *world, double dt, double time)
{
    SolMouse mouse    = Sol_Input_GetMouse();
    u32      required = HAS_PICKUP | HAS_INTERACT;
    for (int i = 0; i < world->activeCount; i++)
    {
        u32 id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;

        CompPickup   *pickup   = &world->pickups[id];
        CompXform    *xform    = &world->xforms[id];
        CompBody     *body     = &world->bodies[id];
        CompInteract *interact = &world->interacts[id];

        if (pickupId == id)
        {
            vec3s targetPos = Sol_ScreenRaycast(world, mouse.x, mouse.y, (SolRay){.dist = 20.0f}).pos;
            body->vel       = vecSca(vecSub(targetPos, xform->pos), stiffness);
            return;
        }

        if (interact->states & INTERACT_PRESSED)
        {
            pickupId = id;
        }
        if (!Sol_Input_GetMouse().buttons[SOL_MOUSE_LEFT])
            pickupId = -1;
    }
}