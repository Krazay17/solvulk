#include "s_body2.h"
#include "world.h"
#include "sol_math.h"

typedef void (*Resolver)(ScBody2 *, ScXform *, ScBody2 *, ScXform *);

static void Resolve_Rect(ScBody2 *body, ScXform *xform, ScBody2 *bodyB, ScXform *xformB);

const Resolver shape_resolver[SHAPE2_CNT][SHAPE2_CNT] = {
    [SHAPE2_REC][SHAPE2_REC] = Resolve_Rect,
};

const vec3s bounds_min = {0.0f, 0.0f, 0.0f};
const vec3s bounds_max = {WINDOW_WIDTH, WINDOW_HEIGHT, 0.0f};

void Body3_Init(World *world)
{
    SysBody2 *ws                   = malloc(sizeof(SysBody2));
    world->systems[WORLDSYS_BODY2] = ws;
    solb_init(ws->contacts, 32);
}

void Body2_Step(World *world, double dt)
{
    float fdt = (float)dt;
    int   i, j;

    SparseSet_ScBody2 *set = Sol_Comp_Set(world, ScBody2);
    for (i = 0; i < set->cnt; i++)
    {
        ScBody2 *body = &set->data[i];
        body->vel     = glms_vec3_scale(body->vel, 0.999f);
        vec3s accel   = body->vel.y > TERMINAL_VELOCITY ? GLMS_VEC3_ZERO : body->gravity;
        accel         = glms_vec3_add(accel, body->force);
        accel         = glms_vec3_add(accel, body->impulse);
        body->impulse = (vec3s){0};
        body->vel     = vecAdd(body->vel, vecSca(accel, fdt));
    }

    for (i = 0; i < set->cnt; i++)
    {
        int      id   = set->dense[i];
        ScBody2 *body = &set->data[i];

        if (!Sol_Comp_Has(world, id, ScXform))
            continue;
        ScXform *xform = Sol_Comp_Get(world, id, ScXform);
        xform->pos     = vecAdd(xform->pos, vecSca(body->vel, fdt));
    }

    // for (i = 0; i < set->cnt; i++)
    // {
    //     int      id   = set->dense[i];
    //     ScBody2 *body = &set->data[i];
    //     if (!Sol_Comp_Has(world, id, ScXform))
    //         continue;
    //     ScXform *xform = Sol_Comp_Get(world, id, ScXform);
    //     for (j = i + 1; j < set->cnt; j++)
    //     {
    //         int      idB   = set->dense[j];
    //         ScBody2 *bodyB = &set->data[j];
    //         if (!Sol_Body2_DoesCollide(body, bodyB) || !Sol_Comp_Has(world, idB, ScXform))
    //             continue;
    //         ScXform *xformB = Sol_Comp_Get(world, idB, ScXform);
    //         if (shape_resolver[body->shape][bodyB->shape])
    //             shape_resolver[body->shape][bodyB->shape](body, xform, bodyB, xformB);
    //     }
    // }

    for (i = 0; i < set->cnt; i++)
    {
        int      id   = set->dense[i];
        ScBody2 *body = &set->data[i];
        if (body->ignoreWindow || !Sol_Comp_Has(world, id, ScXform))
            continue;
        ScXform *xform   = Sol_Comp_Get(world, id, ScXform);
        vec3s    old_pos = xform->pos;
        vec3s    max_pos = glms_vec3_sub(bounds_max, body->dims);

        // Clamp position
        xform->pos = glms_vec3_maxv(bounds_min, glms_vec3_minv(xform->pos, max_pos));

        if (xform->pos.x != old_pos.x)
            body->vel.x = -body->vel.x * body->restitution;
        if (xform->pos.y != old_pos.y)
            body->vel.y = -body->vel.y * body->restitution;
    }
}

static void Resolve_Rect(ScBody2 *body, ScXform *xform, ScBody2 *bodyB, ScXform *xformB)
{
}

vec3s Sol_Body2_AABBPen(ScBody2 *body, ScXform *xform, ScBody2 *bodyB, ScXform *xformB)
{
    vec3s aa = xform->pos;
    vec3s ab = vecAdd(xform->pos, body->dims);

    vec3s ba = xformB->pos;
    vec3s bb = vecAdd(xformB->pos, bodyB->dims);

    vec3s a = vecSub(aa, ba);
    vec3s b = vecSub(ab, bb);

    return vecSub(a, b);
}

int Sol_Body2_GetEntAtPoint(World *world, vec2s point)
{
    int best   = -1;
    int zindex = -1;

    SparseSet_ScBody2 *set = Sol_Comp_Set(world, ScBody2);
    for (int i = 0; i < set->cnt; i++)
    {
        int      id   = set->dense[i];
        ScBody2 *body = &set->data[i];
        if (!Sol_Comp_Has(world, id, ScXform))
            continue;
        ScXform *xform    = Sol_Comp_Get(world, id, ScXform);
        bool     overlapX = (point.x > xform->pos.x) && point.x < (xform->pos.x + body->dims.x);
        bool     overlapY = (point.y > xform->pos.y) && point.y < (xform->pos.y + body->dims.y);
        if (overlapX && overlapY && body->zindex > zindex)
        {
            best   = id;
            zindex = body->zindex;
        }
    }

    return best;
}

bool Sol_Body2_DoesCollide(ScBody2 *body, ScBody2 *bodyB)
{
    return (body->mask << 16) & bodyB->mask;
}