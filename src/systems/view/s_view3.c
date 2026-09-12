#include "world.h"
#include "sol_math.h"
#include "render/render.h"

typedef void (*View3KindDraw)(World *, int, ScView3 *);

static void Sphere_Draw(World *world, int id, ScView3 *view);
static void Fireball_Draw(World *world, int id, ScView3 *view);
static void Healthbar_Draw(World *world, int id, ScView3 *view);

static const View3KindDraw draw_func[VIEW3KIND_COUNT] = {
    [VIEW3KIND_SPHERE]    = Sphere_Draw,
    [VIEW3KIND_FIREBALL]  = Fireball_Draw,
    [VIEW3KIND_HEALTHBAR] = Healthbar_Draw,
};

void View3_Draw(World *world, double dt)
{
    SparseSet_ScView3 *set = Sol_Comp_Set(world, ScView3);
    for (int i = 0; i < set->cnt; i++)
    {
        int id        = set->dense[i];
        ScView3 *view = &set->data[i];

        View3KindDraw func = draw_func[view->kind];

        if (func)
            func(world, id, view);
    }
}

static void Sphere_Draw(World *world, int id, ScView3 *view)
{
    SphereSSBO *sphere = Sol_Render_GetNextSphere(SPHEREKIND_BASIC);
    Xform xform        = Xform_GetDraw(world, id);
    sphere->color      = view->color;
    sphere->pos        = (vec4s){xform.pos.x, xform.pos.y, xform.pos.z, view->dims.x};
}

static void Fireball_Draw(World *world, int id, ScView3 *view)
{
    Xform xform = Xform_GetDraw(world, id);
    vec4s pos   = {xform.pos.x, xform.pos.y, xform.pos.z, view->dims.x};

    *Sol_Render_GetNextSphere(SPHEREKIND_FIREBALL) = (SphereSSBO){
        .color = view->color,
        .pos   = pos,
    };
}

static void Healthbar_Draw(World *world, int id, ScView3 *view)
{
    ScCombat *combat = Sol_Comp_Get(world, id, ScCombat);
    if (!combat)
        return;

    Xform xform   = Xform_GetDraw(world, id);
    vec4s pos     = {xform.pos.x, xform.pos.y, xform.pos.z, 1.0f};
    ScBody3 *body = Sol_Comp_Get(world, id, ScBody3);
    if (body)
        pos.y += body->dims.y;
    float hbHalfWidth  = 1.0f;
    float hbHalfHeight = 0.1f;
    float fill = combat->health <= 0 ? 0 : combat->health / combat->healthMax;

    *Sol_Render_GetNextQuad(QUADKIND_HEALTH) = (QuadSSBO){
        .pos   = pos,
        .rot   = GLMS_VEC4_ZERO,
        .color = view->color,
        .uv    = (vec4s){0, 0, 1, 1},
        .type  = QUADTYPE_FACECAM,
        .rect  = (vec4s){0, 0, hbHalfWidth, hbHalfHeight},
        .extra = (vec4s){0, 0.015f, fill, 0},
    };
}