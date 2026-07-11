#include "s_view.h"
#include "sol_core.h"
#include "sol_engine.h"
#include "world.h"
#include "sol_math.h"
#include "xform/s_xform.h"
#include "model/s_model.h"
#include "combat/s_combat.h"
#include "physx/s_body.h"
#include "render/render.h"
#include "vital/s_vital.h"

#define HEALTHBAR_HIDE_DURATION 5.0f

void        Sol_Crosshair_Draw(World *world, double dt, double time);
static void Healthbar_Draw(World *world, double dt, double time);
static void Weapon_Draw(World *world, double dt, double time);

void Sol_View_Init(World *world)
{
    // Sol_View_Fx(world);
    Sol_Nameplate_Init(world);
    WAdd3d(world) = Weapon_Draw;
}

void Sol_Crosshair_Draw(World *world, double dt, double time)
{
    float     x      = (float)WINDOW_WIDTH / 2.0f;
    float     y      = (float)WINDOW_HEIGHT / 2.0f;
    float     width  = 11.0f;
    float     height = 11.0f;
    RectSSBO *ssbo   = Sol_Render_GetNext_Rect();
    ssbo->pos        = (vec4s){UISCALE(x - width * 0.5f), UISCALE(y - height * 0.5f), 1, 1.0f};
    ssbo->dims       = (vec4s){UISCALE(width), UISCALE(height), 0, 1.0f};
    ssbo->color      = (vec4s){1, 1, 1, 1};
    ssbo->textureID  = SOL_TEXTURE_CROSSHAIR;
    ssbo->uv         = (vec4s){0.0f, 0.0f, 1.0f, 1.0f};
    ssbo->type       = 0;
    ssbo->flags      = 0;
}

static int  draw_required = BITC(HAS_ACTIVE) | BITC(HAS_VITAL);
static void Healthbar_Draw(World *world, double dt, double time)
{
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if (id == 1)
            continue;
        if (Sol_Vital_GetDead(world, id) || !WHas(world, id, draw_required))
            continue;
        if (time - Sol_Vital_GetLastHitTime(world, id) > HEALTHBAR_HIDE_DURATION)
            continue;
        SolXform xform = Sol_Xform_GetDrawXform(world, id);
        xform.pos.y += Sol_Physx_GetDims(world, id).y * 0.77f;

        float health    = Sol_Vital_GetHealth(world, id);
        float maxHealth = Sol_Vital_GetMaxHealth(world, id);
        float fill      = maxHealth > 0 ? health / maxHealth : 0.0f;

        QuadSSBO *ssbo = Sol_Render_GetNext_Quad(QUADKIND_HEALTH);
        if (!ssbo)
            continue;

        float healthbarWidthScale = 1.0f;
        ssbo->pos                 = (vec4s){xform.pos.x, xform.pos.y, xform.pos.z, healthbarWidthScale};
        ssbo->rot                 = GLMS_VEC4_ZERO;
        ssbo->color               = (vec4s){0.1f, 0.85f, 0.2f, 1.0f};
        ssbo->uv                  = (vec4s){0, 0, 1, 1};
        // Shader Parameters Allocation (x = fill percentage, y = border outline scale)
        ssbo->extra = (vec4s){fill, 0.015f, 0, 0};
        // Explicit Subsystem Binding Types
        ssbo->type      = QUADTYPE_FACECAM;
        ssbo->flags     = 0;
        ssbo->textureId = 0;
    }
}

static void Weapon_Draw(World *world, double dt, double time)
{
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if (!(world->masks[id] & BITC(HAS_COMBAT)))
            continue;
        CompModel *model = &world->models[id];

        SolXform leftXform = Sol_Model_GetBoneXform(world, id, "hand.L.Weapon");
        Sol_Xform_SetXform(world, model->leftWeaponEnt, leftXform);

        SolXform rightXform = Sol_Model_GetBoneXform(world, id, "hand.R.Weapon");
        Sol_Xform_SetXform(world, model->rightWeaponEnt, rightXform);
    }
}
