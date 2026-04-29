#include "sol_core.h"

CompSphere *Sol_Sphere_Add(World *world, int id, CompSphere init)
{
    CompSphere sphere = init;

    world->spheres[id] = sphere;
    world->masks[id] |= HAS_SPHERE;
    return &world->spheres[id];
}

void Sphere_Draw(World *world, double dt, double time)
{
    int required = HAS_XFORM | HAS_SPHERE;
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;
        CompSphere *sphere = &world->spheres[id];
        CompXform  *xform  = &world->xforms[id];
        vec4s       pos    = (vec4s){xform->drawPos.x, xform->drawPos.y, xform->drawPos.z, sphere->radius};

        Sol_Submit_Sphere(pos, sphere->color);
    }
}

void Sol_Sphere_ColorAll(World *world, vec4s color)
{
    int required = HAS_SPHERE;
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;

        world->spheres[id].color = color;
    }
}