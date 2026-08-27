#include "body2d_i.h"
#include "sol_core.h"
#include "world.h"
#include "sol_math.h"
#include "input.h"

#include "xform/s_xform.h"
#include "parent/s_parent.h"
#include "interact/s_interact.h"
#include "render/render.h"
#include "s_body2d.h"

static void Body2_Step(World *world, double dt, double time)
{
    static int required = BITC(HAS_ACTIVE) | BITC(HAS_BODY2);

    float fdt = (float)dt;
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if (!WHas(world, id, required))
            continue;

        CompXform  *xform = &world->xforms[id];
        CompBody2d *body  = &world->body2d[id];

        // Accumulate velocity
        body->vel = ApplyFriction2((vec2s){0, 0}, body->vel, 1.0f, fdt);
        body->vel = glms_vec2_add(body->vel, glms_vec2_scale(body->grav, fdt));

        // Apply velocity
        xform->pos.x += body->vel.x;
        xform->pos.y += body->vel.y;
    }
    // Resolve collision
    for (int i = 0; i < world->activeCount; i++)
    {
        int idA = world->activeEntities[i];
        if (!WHas(world, idA, required))
            continue;
        CompBody2d *bodyA  = &world->body2d[idA];
        CompXform  *xformA = &world->xforms[idA];

        for (int j = i + 1; j < world->activeCount; j++)
        {
            int idB = world->activeEntities[j];
            if (!WHas(world, idB, required))
                continue;

            CompBody2d *bodyB = &world->body2d[idB];

            bool layersMatch = Sol_Body2d_DoesCollide(world, idA, idB);

            if (!layersMatch)
                continue;
            CompXform *xformB = &world->xforms[idB];

            vec2s posA = {xformA->pos.x, xformA->pos.y};
            vec2s posB = {xformB->pos.x, xformB->pos.y};

            if (resolver_kinds[bodyA->kind])
                resolver_kinds[bodyA->kind](world, &posA, bodyA, &posB, bodyB);

            xformA->pos = (vec3s){posA.x, posA.y, 0};
            xformB->pos = (vec3s){posB.x, posB.y, 0};
        }
    }
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if (!WHas(world, id, required))
            continue;
        CompBody2d *body  = &world->body2d[id];
        CompXform  *xform = &world->xforms[id];
        vec2s       pos   = {xform->pos.x, xform->pos.y};

        CollideScreenEdge(&body->vel, &pos, body->dims);
        xform->pos = (vec3s){pos.x, pos.y, 0};
    }
}

static void Body2_Draw(World *world, double dt, double time)
{
    if (!solState.debug)
        return;
    static int required = BITC(HAS_ACTIVE) | BITC(HAS_BODY2);

    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if (!WHas(world, id, required))
            continue;

        CompXform  *xform = &world->xforms[id];
        CompBody2d *body  = &world->body2d[id];

        RectSSBO *ssbo = Sol_Render_GetNext_Rect();
        ssbo->rect =
            (vec4s){UISCALE(xform->drawPos.x), UISCALE(xform->drawPos.y), UISCALE(body->dims.x), UISCALE(body->dims.y)};
        ssbo->color            = VEC4_RED;
        ssbo->scale            = 1.0f;
        ssbo->border           = 4.0f;
        ssbo->fill             = 1.0f;
        CompInteract *interact = Sol_Interact_Get(world, id);
        if (interact && interact->state & INTERACT_HOVERED)
            ssbo->color = VEC4_WHITE;
    }
}

void Sol_Body2d_Init(World *world)
{
    world->body2d   = calloc(MAX_ENTS, sizeof(CompBody2d));
    WAddStep(world) = Body2_Step;
    WAdd2d(world)   = Body2_Draw;
}

CompBody2d *Sol_Body2d_Add(World *world, int id, Shape2 kind, float width, float height, u32 group)
{
    CompBody2d body = {
        .kind   = kind,
        .dims.x = width,
        .dims.y = height,
        .group  = group,
    };
    world->body2d[id] = body;
    world->masks[id] |= BITC(HAS_BODY2);

    return &world->body2d[id];
}

CompBody2d *Sol_Body2d_Get(World *world, int id)
{
    if (!WHasB(world, id, HAS_BODY2))
        return NULL;
    return &world->body2d[id];
}

vec2s Sol_Body2d_GetDims(World *world, int id)
{
    return world->body2d[id].dims;
}

void Sol_Body2d_SetOverlapMask(World *world, int id, u32 group)
{
    world->body2d[id].overlap_group = group;
}
void Sol_Body2d_SetVel(World *world, int id, vec2s vel)
{
    CompBody2d *body = &world->body2d[id];
    body->vel        = vel;
}

int Sol_Body2d_GetOverlapping(World *world, int id, int *overlapping_ents, int max)
{
    static int required = BITC(HAS_ACTIVE) | BITC(HAS_BODY2);
    if (!WHas(world, id, required))
        return 0;
    int count = 0;
    for (int i = 0; i < world->activeCount; i++)
    {
        int idB = world->activeEntities[i];
        if (!WHas(world, idB, required))
            continue;
        if (IsOverlappingRect(world, id, idB))
            overlapping_ents[count++] = idB;
    }
    return count;
}

bool Sol_Body2d_DoesCollide(World *world, int id, int idB)
{
    return world->body2d[id].group << 16 & world->body2d[idB].group;
}