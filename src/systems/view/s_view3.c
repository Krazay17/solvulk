#include "world.h"
#include "sol_math.h"
#include "render/render.h"
#include "sol_user.h"

static void Sphere_Draw(World *world, int id, const View3 *view)
{
    Xform xform        = Xform_GetDraw(world, id);
    SphereSSBO *sphere = Sol_Render_GetNextSphere(PIPE_SPHERE);
    sphere->pos        = (vec4s){xform.pos.x, xform.pos.y, xform.pos.z, view->scale};
    sphere->color      = view->color;
}

static void Bolt_Draw(World *world, int id, const View3 *view)
{
    Xform xform = Xform_GetDraw(world, id);

    vec4s pos4 = {xform.pos.x, xform.pos.y, xform.pos.z, xform.sca.x};
    vec4s sca4 = {xform.sca.x, xform.sca.y, xform.sca.z, 1.0f};
    vec4s rot4 = {xform.rot.x, xform.rot.y, xform.rot.z, xform.rot.w};

    vec4s model_sca      = {0.5f, 1.0f, 0.5f, 1.0f};
    ModelSSBO model_ssbo = {
        .color    = {1.0f, 1.0f, 1.0f, 1.0f},
        .position = pos4,
        .scale    = glms_vec4_scale(model_sca, xform.sca.x),
        .rotation = rot4,
    };
    Sol_Render_PushModel(MODELKIND_BOLT, &model_ssbo, NULL, true);

    versors roll_90   = glms_quatv(GLM_PI_2f, (vec3s){0.0f, 1.0f, 0.0f});
    versors cross_rot = glms_quat_mul(to_versors(rot4), roll_90);
    for (int i = 0; i < 2; i++)
    {
        QuadSSBO *quad  = Sol_Render_GetNextQuad(PIPE_QUAD_ADD);
        quad->textureId = SOL_TEXTURE_SHOCKSPRITE4;
        quad->type      = 1;
        quad->pos       = pos4;
        quad->rot       = i == 0 ? rot4 : to_vec4s(cross_rot);
        quad->color     = VEC4_WHITE;
        quad->rect      = (vec4s){0, 0, 1.0f, 3.0f};
        u32 sprite_anim = (u32)floorf(world->tickTime * 10.0f) % 4;
        quad->uv        = SPRITEPAGE4[sprite_anim];
    }
}

static void Fireball_Draw(World *world, int id, const View3 *view)
{
    Xform xform = Xform_GetDraw(world, id);
    float scale = view->scale;

    forc(world, id, ScProjectile) scale = c->radius;

    vec4s pos = {xform.pos.x, xform.pos.y, xform.pos.z, scale};

    *Sol_Render_GetNextSphere(PIPE_FIREBALL) = (SphereSSBO){
        .color = view->color,
        .pos   = pos,
    };
}

static void Powerbar_Draw(World *world, int id, const View3 *view)
{
    ScCombat *combat = Sol_Comp_Get(world, id, ScCombat);

    if (!combat || combat->health <= 0.0f)
        return;

    if (id == sol_user.view_ent)
        return;

    vec3s player_pos = world->xform.pos[sol_user.view_ent];
    Xform xform      = Xform_GetDraw(world, id);
    vec4s pos4       = {xform.pos.x, xform.pos.y, xform.pos.z, 1.0f};
    ScBody3 *body    = Sol_Comp_Get(world, id, ScBody3);
    if (body)
        pos4.y += body->dims.y;

    float fill  = glm_clamp(combat->health / combat->healthMax, 0.0f, 1.0f);
    float fillB = glm_clamp(combat->energy / combat->energyMax, 0.0f, 1.0f);

    *Sol_Render_GetNextQuad(PIPE_HEALTHBAR) = (QuadSSBO){
        .pos   = pos4,
        .rect  = (vec4s){0.0f, 0.0f, 2.0f, 0.2f},
        .color = view->color,
        .uv    = (vec4s){1.0f, 1.0f, 0.0f, 0.0f},
        .extra = (vec4s){fill, fillB, 0.0f, 0.0f},
        .type  = QUADTYPE_FACECAM,
    };
    forc(world, id, ScMeta)
    {
        const float name_size = 0.25f;

        *Sol_Render_GetNextQuad(PIPE_QUAD) = (QuadSSBO){
            .pos   = pos4,
            .rect  = (vec4s){0.0f, 0.225f, Sol_MeasureText3D(c->name, name_size, SOL_FONT_ICE), name_size},
            .color = VEC4_BLACK,
            .uv    = (vec4s){1.0f, 1.0f, 0.0f, 0.0f},
            .type  = QUADTYPE_FACECAM,
        };
        vec3s pos = {pos4.x, pos4.y, pos4.z};
        pos.y += 0.125f;
        Sol_Render_DrawText3D(c->name, (Text3DDesc){.pos       = pos,
                                                    .size      = 0.25f,
                                                    .color     = VEC4_GREEN,
                                                    .billboard = true,
                                                    .inFront   = true,
                                                    .font      = SOL_FONT_ICE});
    }
}

static void (*const draw_func[VIEW3KIND_COUNT])(World *, int, const View3 *) = {
    [VIEW3KIND_SPHERE]   = Sphere_Draw,
    [VIEW3KIND_FIREBALL] = Fireball_Draw,
    [VIEW3KIND_POWERBAR] = Powerbar_Draw,
    [VIEW3KIND_BOLT]     = Bolt_Draw,
};

void View3_Draw(World *world, double dt)
{
    SparseSet_ScView3 *set = Sol_Comp_Set(world, ScView3);
    for (int i = 0; i < set->cnt; i++)
    {
        int id      = set->dense[i];
        ScView3 *sc = &set->data[i];

        for (int j = 0; j < solb_count(sc->views_b); j++)
        {
            View3 *view = &sc->views_b[j];
            view->_elapsed += dt;
            if ((view->duration > 0) && view->_elapsed >= view->duration)
                continue;
            draw_func[view->kind] ? draw_func[view->kind](world, id, view) : Sphere_Draw(world, id, view);
        }
    }
}
