#include "body2d_i.h"
#include "world.h"
#include "sol_math.h"
#include "input.h"
#include "xform/s_xform.h"
#include "parent/s_parent.h"
#include "interact/s_interact.h"

static void Step(World *world, double dt, double time);

void Sol_Body2d_Init(World *world)
{
    world->body2d   = calloc(MAX_ENTS, sizeof(CompBody2d));
    WAddStep(world) = Step;
}

CompBody2d *Sol_Body2d_Add(World *world, int id, Body2dKind kind, float width, float height, u32 group, u32 mask)
{
    CompBody2d body = {
        .kind   = kind,
        .dims.x = width,
        .dims.y = height,
        .group  = group,
        .mask   = mask,
    };
    world->body2d[id] = body;
    world->masks[id] |= BITC(HAS_BODY2);

    return &world->body2d[id];
}

static int   step_required = BITC(HAS_BODY2);
static void Step(World *world, double dt, double time)
{
    float fdt      = (float)dt;

    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if (!WHas(world, id, step_required))
            continue;

        CompXform  *xform = &world->xforms[id];
        CompBody2d *body  = &world->body2d[id];

        // Accumulate velocity
        body->vel = ApplyFriction2((vec2s){0, 0}, body->vel, 1.0f, fdt);
        if (world->masks[id] & BITC(HAS_INTERACT) && world->interacts[id].state & INTERACT_MOVING)
        {
            CompInteract *interact = &world->interacts[id];
            if (world->masks[id] & BITC(HAS_PARENT))
                Sol_Parent_SetActive(world, id, false);
            vec2s       grabPos   = glms_vec2_add((vec2s){xform->pos.x, xform->pos.y}, interact->grabOffset);
            vec2s       toMouse   = glms_vec2_sub(Sol_Input_GetMouseUI(), grabPos);
            const float stiffness = 80.0f;
            float       alpha     = 1.0f - expf(-stiffness * fdt);
            body->vel             = glms_vec2_scale(toMouse, alpha);
        }
        body->vel = glms_vec2_add(body->vel, glms_vec2_scale(body->grav, fdt));

        // Apply velocity
        xform->pos.x += body->vel.x;
        xform->pos.y += body->vel.y;
    }
    // Resolve collision
    for (int i = 0; i < world->activeCount; i++)
    {
        int idA = world->activeEntities[i];
        if ((world->masks[idA] & step_required) != step_required)
            continue;
        CompBody2d *bodyA  = &world->body2d[idA];
        CompXform  *xformA = &world->xforms[idA];

        for (int j = i + 1; j < world->activeCount; j++)
        {
            int idB = world->activeEntities[j];
            if ((world->masks[idB] & step_required) != step_required)
                continue;

            CompBody2d *bodyB       = &world->body2d[idB];
            bool        layersMatch = (bodyA->mask & bodyB->group) && (bodyB->mask & bodyA->group);
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
        if ((world->masks[id] & step_required) != step_required)
            continue;
        CompBody2d *body  = &world->body2d[id];
        CompXform  *xform = &world->xforms[id];
        vec2s       pos   = {xform->pos.x, xform->pos.y};

        CollideScreenEdge(&body->vel, &pos, body->dims);
        xform->pos = (vec3s){pos.x, pos.y, 0};
    }
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & step_required) != step_required)
            continue;
        CompBody2d *bodyA = &world->body2d[id];
        int         count = 0;
        for (int j = 0; j < world->activeCount; j++)
        {
            int idB = world->activeEntities[j];
            if (idB == id)
                continue;
            if ((world->masks[idB] & step_required) != step_required)
                continue;
            if (IsOverlappingRect(world, id, idB) && count < 4)
                bodyA->overlapping[count++] = idB;
        }
        bodyA->overlapCount = count;
    }
}

vec2s Sol_Body2d_GetDims(World *world, int id)
{
    return world->body2d[id].dims;
}

void Sol_Body2d_SetOverlapMask(World *world, int id, u32 group, u32 mask)
{
    world->body2d[id].overlapGroup = group;
    world->body2d[id].overlapMask  = mask;
}