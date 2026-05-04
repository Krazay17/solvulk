#include "sol_core.h"
#include "xform/xform_system.h"

typedef struct CompSphere
{
    float radius;
    vec4s color;
} CompSphere;

void Sol_Sphere_Add(World *world, int id, SphereDesc desc)
{
    world->masks[id] |= HAS_SPHERE;
    CompSphere sphere = {
        .radius = desc.radius,
        .color = desc.color,
    };
    world->spheres[id] = sphere;
}

void Sol_Sphere_Step(World *world, double dt, double time)
{
    SolEvents *s = world->events;
    if (!s)
        return;
    for (int i = 0; i < s->count; i++)
    {
        SolEvent *e = &s->event[i];
        if (!(world->masks[e->entA] & HAS_SPHERE))
            continue;
        if (e->kind != EVENT_COLLISION)
            continue;
        if (glms_vec3_norm(Sol_Physx_GetVel(world, e->entA)) < 1.0f)
            continue;

        Emitter_Add(world, (Emitter){.pos      = e->pos,
                                     .burst    = 10,
                                     .rate     = 0,
                                     .particle = (Particle){.color = (vec4s){255, 0, 0, 155}, .ttl = 0.6f, .scale=0.2f}});
        e->kind = 0;
    }
}

void Sol_Sphere_Draw(World *world, double dt, double time)
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

        Sol_Draw_Sphere(pos, sphere->color);
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