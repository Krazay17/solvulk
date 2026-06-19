/*
 * File: s_sphere.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * Spheres!
 */
#include "s_shape.h"
#include "world.h"
#include "xform/s_xform.h"
#include "render/render.h"

void Sol_Shape_Init(World *world)
{
    world->draw3dSystems[world->draw3dCount++] = Sol_Shape_Draw;

    world->spheres = calloc(MAX_ENTS, sizeof(CompShape));
}

void Sol_Shape_Add(World *world, int id, ShapeDesc desc)
{
    world->masks[id] |= HAS_SHAPE;
    CompShape sphere = {
        .kind   = desc.kind,
        .radius = desc.radius,
        .color  = desc.color,
    };
    world->spheres[id] = sphere;
}

void Sol_Shape_Draw(World *world, double dt, double time)
{
    int required = HAS_SHAPE;
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;
        CompShape *shape = &world->spheres[id];
        CompXform *xform = &world->xforms[id];
        vec4s      pos   = (vec4s){xform->drawPos.x, xform->drawPos.y, xform->drawPos.z, shape->radius};
        switch (shape->kind)
        {
        case SHAPEKIND_FIREBALL: {
            SphereSSBO *p = Sol_Render_GetNext_Fireball();
            p->pos = pos;
            p->color = shape->color;
        }
        break;

        default: {
            SphereSSBO *o = Sol_Render_GetNext_Sphere(false);
            o->pos = pos;
            o->color = shape->color;
        }
        break;
        }
    }
}

void Sol_Shape_ColorAll(World *world, vec4s color)
{
    int required = HAS_SHAPE;
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;

        world->spheres[id].color = color;
    }
}