#include "world.h"
#include "render/render.h"

typedef void (*View3KindDraw)(World *, int, ScView3 *);

static void Sphere_Draw(World *world, int id, ScView3 *view);
static void Fireball_Draw(World *world, int id, ScView3 *view);

static const View3KindDraw draw_funcs[VIEW3KIND_COUNT] = {
    [VIEW3KIND_SPHERE]   = Sphere_Draw,
    [VIEW3KIND_FIREBALL] = Sphere_Draw,
};

void View3_Draw(World *world, double dt)
{
    SparseSet_ScView3 *set = Sol_Comp_Set(world, ScView3);
    for (int i = 0; i < set->cnt; i++)
    {
        int id        = set->dense[i];
        ScView3 *view = &set->data[i];

        SphereKind sphereKind = SPHEREKIND_BASIC;
        switch (view->kind)
        {
        case VIEW3KIND_FIREBALL:
            sphereKind = SPHEREKIND_FIREBALL;
            break;
        }
        SphereSSBO *sphere = Sol_Render_GetNextSphere(sphereKind);
        Xform xform        = Xform_GetDraw(world, id);
        sphere->color      = view->color;
        sphere->pos        = (vec4s){xform.pos.x, xform.pos.y, xform.pos.z, view->dims.x};

        // if (draw_funcs[view->kind])
        //     draw_funcs[view->kind](world, id, view);
    }
}

static void Sphere_Draw(World *world, int id, ScView3 *view)
{
    SphereSSBO *sphere = Sol_Render_GetNextSphere(SPHEREKIND_BASIC);
    Xform xform        = Xform_GetDraw(world, id);
    sphere->color      = view->color;
    sphere->pos        = (vec4s){xform.pos.x, xform.pos.y, xform.pos.z, view->dims.x};
}