#include "s_body2.h"
#include "world.h"
#include "sol_math.h"

typedef void (*Resolver)(World *, ScBody2 *, ScBody2 *);

static void Resolve_Rect(World *world, ScBody2 *body, ScBody2 *bodyB);

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
    int i, j;

    SparseSet_ScBody2 *set = Sol_Comp_Set(world, ScBody2);
    for (i = 0; i < set->cnt; i++)
    {
        ScBody2 *body = &set->data[i];
        body->vel     = glms_vec3_scale(body->vel, 0.966f);
        vec3s accel   = body->vel.y > TERMINAL_VELOCITY ? GLMS_VEC3_ZERO : body->gravity;
        accel         = glms_vec3_add(accel, body->force);
        accel         = glms_vec3_add(accel, body->impulse);
        body->impulse = (vec3s){0};
        body->vel     = vecAdd(body->vel, vecSca(accel, fdt));
    }

    for (i = 0; i < set->cnt; i++)
    {
        int id        = set->dense[i];
        ScBody2 *body = &set->data[i];

        world->xform.pos[id] = vecAdd(world->xform.pos[id], vecSca(body->vel, fdt));
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
        int id        = set->dense[i];
        ScBody2 *body = &set->data[i];
        if (body->ignoreWindow)
            continue;
            Xforms xform = Xform_Get(world, id);
        vec3s old_pos  = xform.pos;
        vec3s max_pos  = glms_vec3_sub(bounds_max, body->dims);

        // Clamp position
        world->xform.pos[id] = glms_vec3_maxv(bounds_min, glms_vec3_minv(old_pos, max_pos));

        if (world->xform.pos[id].x != old_pos.x)
            body->vel.x = -body->vel.x * body->restitution;
        if (world->xform.pos[id].y != old_pos.y)
            body->vel.y = -body->vel.y * body->restitution;
    }
}

bool Sol_Body2_ContainsPoint(World *world, int id, vec2s point)
{
    ScBody2 *body = Sol_Comp_Get(world, id, ScBody2);
    bool overlapX = (point.x > world->xform.pos[id].x) && point.x < (world->xform.pos[id].x + body->dims.x);
    bool overlapY = (point.y > world->xform.pos[id].y) && point.y < (world->xform.pos[id].y + body->dims.y);
    if (overlapX && overlapY)
        return true;

    return false;
}

int Sol_Body2_GetEntAtPoint(World *world, vec2s point)
{
    int best   = -1;
    int zindex = -1;

    SparseSet_ScBody2 *set = Sol_Comp_Set(world, ScBody2);
    for (int i = 0; i < set->cnt; i++)
    {
        int id        = set->dense[i];
        ScBody2 *body = &set->data[i];
        bool overlapX = (point.x > world->xform.pos[id].x) && point.x < (world->xform.pos[id].x + body->dims.x);
        bool overlapY = (point.y > world->xform.pos[id].y) && point.y < (world->xform.pos[id].y + body->dims.y);
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

static void Resolve_Rect(World *world, ScBody2 *body, ScBody2 *bodyB)
{

}