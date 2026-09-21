#include "world.h"
#include "sol_math.h"
#include "render/render.h"
#include "sol_user.h"

typedef void (*View3KindDraw)(World *, int, ScView3 *);

static void Sphere_Draw(World *world, int id, ScView3 *view)
{
    Xform xform        = Xform_GetDraw(world, id);
    SphereSSBO *sphere = Sol_Render_GetNextSphere(PIPE_SPHERE);
    sphere->pos        = (vec4s){xform.pos.x, xform.pos.y, xform.pos.z, view->scale};
    sphere->color      = view->color;
}

static void DragonOrb_Draw(World *world, int id, ScView3 *view)
{
    Xform xform        = Xform_GetDraw(world, id);
    SphereSSBO *sphere = Sol_Render_GetNextSphere(PIPE_PARTICLE_DRAGON);
    sphere->pos        = (vec4s){xform.pos.x, xform.pos.y, xform.pos.z, view->scale};
    sphere->color      = view->color;
}

static void PlasmaOrb_Draw(World *world, int id, ScView3 *view)
{
    Xform xform        = Xform_GetDraw(world, id);
    SphereSSBO *sphere = Sol_Render_GetNextSphere(PIPE_PLASMA);
    sphere->pos        = (vec4s){xform.pos.x, xform.pos.y, xform.pos.z, view->scale};
    sphere->color      = view->color;
}

static void Fireball_Draw(World *world, int id, ScView3 *view)
{
    Xform xform = Xform_GetDraw(world, id);
    vec4s pos   = {xform.pos.x, xform.pos.y, xform.pos.z, view->scale};

    *Sol_Render_GetNextSphere(PIPE_FIREBALL) = (SphereSSBO){
        .color = view->color,
        .pos   = pos,
    };
}

static void Healthbar_Draw(World *world, int id, ScView3 *view)
{
    ScCombat *combat = Sol_Comp_Get(world, id, ScCombat);

    if (!combat || combat->health <= 0.0f)
        return;

    if (id == sol_user.view_ent)
        return;
    vec3s player_pos = world->xform.pos[sol_user.view_ent];
    Xform xform      = Xform_GetDraw(world, id);
    vec4s pos        = {xform.pos.x, xform.pos.y, xform.pos.z, 1.0f};
    if (glms_vec3_norm(vecSub(xform.pos, player_pos)) > 15.0f)
        return;
    ScBody3 *body = Sol_Comp_Get(world, id, ScBody3);
    if (body)
        pos.y += body->dims.y;

    float fill = combat->health / combat->healthMax;

    *Sol_Render_GetNextQuad(PIPE_HEALTHBAR) = (QuadSSBO){
        .pos   = pos,
        .rect  = (vec4s){0.0f, 0.0f, 2.0f, 0.2f},
        .color = view->color,
        .uv    = (vec4s){0.0f, 0.0f, 1.0f, 1.0f},
        .extra = (vec4s){fill, 0.0f, 0.0f, 0.0f},
        .type  = QUADTYPE_FACECAM,
    };
}

static void Pyramid_Draw(World *world, int id, ScView3 *view)
{
    Xform xform = Xform_GetDraw(world, id);
    vec4s pos   = {xform.pos.x, xform.pos.y, xform.pos.z, view->scale};

    QuadSSBO *push = Sol_Render_GetNextQuad(PIPE_FRACTAL_PYRAMID);
    *push          = (QuadSSBO){
        .pos   = (vec4s){pos.x, pos.y, pos.z, view->scale},
        .rect  = {0, 0, 7.0f, 7.0f},
        .color = {1, 1, 1, 1},
        .uv    = (vec4s){0.0f, 0.0f, 1.0f, 1.0f},
    };
}

static const View3KindDraw draw_func[VIEW3KIND_COUNT] = {
    [VIEW3KIND_SPHERE]    = Sphere_Draw,
    [VIEW3KIND_FIREBALL]  = Fireball_Draw,
    [VIEW3KIND_HEALTHBAR] = Healthbar_Draw,
    [VIEW3KIND_PYRAMID]   = Pyramid_Draw,
};

void View3_Draw(World *world, double dt)
{
    SparseSet_ScView3 *set = Sol_Comp_Set(world, ScView3);
    for (int i = 0; i < set->cnt; i++)
    {
        int id        = set->dense[i];
        ScView3 *view = &set->data[i];

        // View3KindDraw func = draw_func[view->kind];

        switch (view->kind)
        {
        case VIEW3KIND_FIREBALL:
            Fireball_Draw(world, id, view);
            break;
        case VIEW3KIND_SPHERE:
            Sphere_Draw(world, id, view);
            break;
        case VIEW3KIND_HEALTHBAR:
            Healthbar_Draw(world, id, view);
            break;
        case VIEW3KIND_PYRAMID:
            Pyramid_Draw(world, id, view);
            break;
        case VIEW3KIND_DRAGONORB:
            DragonOrb_Draw(world, id, view);
            break;
        case VIEW3KIND_PLASMAORB:
            PlasmaOrb_Draw(world, id, view);
            break;
        }
        // if (func)
        //     func(world, id, view);
    }
}
