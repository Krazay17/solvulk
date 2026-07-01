#include "s_view.h"
#include "world.h"
#include "sol_math.h"
#include "buff/s_buff.h"
#include "vital/s_vital.h"
#include "physx/s_body.h"
#include "render/render.h"

#define HEALTHBAR_HIDE_DURATION 5.0f

struct BuffIconConfig
{
    float width, height, spacing;
} buff_icon_config = {
    .width   = 5.0f,
    .height  = 6.0f,
    .spacing = 5.0f,
};

static void Nameplate_Draw(World *world, double dt, double time);

void Sol_Nameplate_Init(World *world)
{
    WAdd3d(world) = Nameplate_Draw;
}

static void Nameplate_Draw(World *world, double dt, double time)
{
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if (id == 1)
            continue;
        if (WHas(world, id, BITC(HAS_VITAL)) && !Sol_Vital_GetDead(world, id) &&
            (time - Sol_Vital_GetLastHitTime(world, id) < HEALTHBAR_HIDE_DURATION))
        {
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
            ssbo->extra     = (vec4s){fill, 0.015f, 0, 0};
            ssbo->type      = QUADTYPE_FACECAM;
            ssbo->flags     = 0;
            ssbo->textureId = 0;
        }

        // if (WHas(world, id, BITC(HAS_BUFF)))
        // {
        //     CompBuff *buffs = &world->buffs[id];
        //     for (int b = 0; b < buffs->count; b++)
        //     {
        //         Buff *buff = &buffs->buffs[b];
        //     }
        // }
    }
}
