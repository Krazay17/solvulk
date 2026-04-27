#include "sol_core.h"

CompBody *Entity_Add_Body2(World *world, int id)
{
    world->masks[id] |= HAS_BODY2;
    return &world->bodies[id];
}

void Sol_System_Step_Physx_2d(World *world, double dt, double time)
{
    float fdt      = (float)dt;
    int   required = HAS_BODY2 | HAS_XFORM;
    for (int i = 0; i < world->activeCount; ++i)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) == required)
        {
            CompBody     *body     = &world->bodies[id];
            CompMovement *movement = &world->movements[id];
            CompXform    *xform    = &world->xforms[id];

            float moveGrav = MOVE_STATE_FORCES[movement->configId][movement->moveState].gravity;
            body->vel.y    = moveGrav ? body->vel.y + moveGrav * fdt : body->vel.y + SOL_PHYS_GRAV.y * fdt;
            vec3s finalVel = glms_vec3_scale(body->vel, fdt * 20.0f);
            xform->pos     = glms_vec3_add(xform->pos, finalVel);

            if (xform->pos.y + body->height >= Sol_GetState()->windowHeight)
            {
                xform->pos.y = Sol_GetState()->windowHeight - body->height;
                body->vel.y  = 0;
            }
        }
    }
}
