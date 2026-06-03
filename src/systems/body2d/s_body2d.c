#include "sol_core.h"

#include "body2d_i.h"

static void Step(World *world, double dt, double time);

void Sol_Body2d_Init(World *world)
{
    world->body2d   = calloc(MAX_ENTS, sizeof(CompBody2d));
    WAddStep(world) = Step;
}

void Sol_Body2d_Add(World *world, int id, CompBody2d desc)
{
    world->masks[id] |= HAS_BODY2;
    world->body2d[id] = desc;
}

static void Step(World *world, double dt, double time)
{
    float fdt      = (float)dt;
    int   required = HAS_BODY2;

    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;

        CompXform  *xform = &world->xforms[id];
        CompBody2d *body  = &world->body2d[id];

        // Accumulate velocity
        body->vel = ApplyFriction2((vec2s){0, 0}, body->vel, 1.0f, fdt);
        if (world->masks[id] & HAS_INTERACT && world->interacts[id].state & INTERACT_MOVING)
            Grab(&body->vel, (vec2s){xform->pos.x, xform->pos.y}, body->dims);
        body->vel = glms_vec2_add(body->vel, glms_vec2_scale(body->grav, fdt));

        // Apply velocity
        xform->pos.x += body->vel.x;
        xform->pos.y += body->vel.y;
    }
    // Resolve collision
    for (int i = 0; i < world->activeCount; i++)
    {
        int idA = world->activeEntities[i];
        if ((world->masks[idA] & required) != required)
            continue;

        for (int j = i + 1; j < world->activeCount; j++)
        {
            int idB = world->activeEntities[j];
            if ((world->masks[idB] & required) != required)
                continue;

            CompBody2d *bodyA  = &world->body2d[idA];
            CompXform  *xformA = &world->xforms[idA];
            CompBody2d *bodyB  = &world->body2d[idB];
            CompXform  *xformB = &world->xforms[idB];

            vec2s posA = {xformA->pos.x, xformA->pos.y};
            vec2s posB = {xformB->pos.x, xformB->pos.y};

            resolver_kinds[bodyA->kind](world, &posA, bodyA, &posB, bodyB);

            xformA->pos = (vec3s){posA.x, posA.y, 0};
            xformB->pos = (vec3s){posB.x, posB.y, 0};
        }
    }
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;

        CompBody2d *body  = &world->body2d[id];
        CompXform  *xform = &world->xforms[id];
        vec2s       pos   = {xform->pos.x, xform->pos.y};

        CollideScreenEdge(&body->vel, &pos, body->dims);
        xform->pos = (vec3s){pos.x, pos.y, 0};
    }
}

vec2s Sol_Body2d_GetDims(World *world, int id)
{
    return world->body2d[id].dims;
}