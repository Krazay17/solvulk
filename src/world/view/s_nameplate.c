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
    .width   = 0.4f,
    .height  = 0.4f,
    .spacing = 0.02f,
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

        if (!WHas(world, id, BITC(HAS_VITAL)) || Sol_Vital_GetDead(world, id))
            continue;

        SolXform anchor = Sol_Xform_GetDrawXform(world, id);
        anchor.pos.y += Sol_Physx_GetDims(world, id).y * 0.77f;
        vec4s anchor4 = (vec4s){anchor.pos.x, anchor.pos.y, anchor.pos.z, 1.0f};

        float health    = Sol_Vital_GetHealth(world, id);
        float maxHealth = Sol_Vital_GetMaxHealth(world, id);
        float fill      = maxHealth > 0 ? health / maxHealth : 0.0f;

        QuadSSBO *ssbo = Sol_Render_GetNext_Quad(QUADKIND_HEALTH);
        if (!ssbo)
            continue;

        ssbo->pos       = anchor4;
        ssbo->rot       = GLMS_VEC4_ZERO;
        ssbo->color     = (vec4s){0.1f, 0.85f, 0.2f, 1.0f};
        ssbo->uv        = (vec4s){0, 0, 1, 1};
        ssbo->type      = QUADTYPE_FACECAM;
        ssbo->flags     = 0;
        ssbo->textureId = 0;

        float hbHalfWidth  = 1.0f;
        float hbHalfHeight = 0.1f;
        ssbo->rect         = (vec4s){0, 0, hbHalfWidth, hbHalfHeight};
        ssbo->extra        = (vec4s){0, 0.015f, fill, 0};

        if (WHas(world, id, BITC(HAS_BUFF)))
        {
            CompBuff *buffs = &world->buffs[id];
            if (buffs->count == 0)
                continue;

            float iconWidth  = buff_icon_config.width;
            float iconHeight = buff_icon_config.height;
            float spacing    = buff_icon_config.spacing;

            float iconHW = iconWidth * 0.5f;
            float iconHH = iconHeight * 0.5f;

            // 1. Calculate a fixed local vertical gap above the healthbar center
            float localOffsetY = hbHalfHeight + iconHH;

            // 2. Compute the starting local horizontal center offset
            float totalWidth  = (buffs->count * iconWidth) + ((buffs->count - 1) * spacing);
            float startLocalX = -(totalWidth * 0.5f) + iconHW;

            for (int b = 0; b < buffs->count; b++)
            {
                Buff *buff = &buffs->buffs[b];

                // Calculate the 2D offset relative to the billboard center point
                float     localOffsetX = startLocalX + b * (iconWidth + spacing);
                QuadSSBO *bg           = Sol_Render_GetNext_Quad(QUADKIND_SPRITE_FRONT);
                if (bg)
                {
                    bg->pos   = anchor4;
                    bg->color = (vec4s){0.0f, 0.0f, 0.0f, 1.0f};
                    bg->rect  = (vec4s){localOffsetX, localOffsetY, iconHW, iconHH};
                }

                QuadSSBO *border = Sol_Render_GetNext_Quad(QUADKIND_SPRITE_FRONT);
                if (border)
                {
                    border->pos   = anchor4;
                    border->color = buff->harmful ? (vec4s){0.5f, 0.0f, 0.0f, 1.0f} : (vec4s){0.0f, 0.0f, 1.0f, 1.0f};
                    border->rect  = (vec4s){localOffsetX, localOffsetY, iconHW, iconHH};
                    border->textureId = SOL_TEXTURE_SPIKEFRAMEFILLED;
                    border->uv        = (vec4s){0, 0, 1, 1};
                }

                // Draw Icon Quad
                QuadSSBO *buffIcon = Sol_Render_GetNext_Quad(QUADKIND_SPRITE_FRONT);
                if (buffIcon)
                {
                    buffIcon->pos   = anchor4;
                    buffIcon->color = (vec4s){1.0f, 1.0f, 1.0f, 1.0f};
                    buffIcon->type  = QUADTYPE_FACECAM;

                    buffIcon->rect = (vec4s){localOffsetX, localOffsetY, iconHW, iconHH};

                    if (buff_icon_map[buff->kind])
                    {
                        buffIcon->textureId = buff_icon_map[buff->kind];
                        buffIcon->uv        = (vec4s){0, 0, 1, 1};
                    }
                }

                QuadSSBO *cdBar = Sol_Render_GetNext_Quad(QUADKIND_SPRITE_FRONT);
                if (cdBar)
                {
                    cdBar->pos       = anchor4;
                    cdBar->rect      = (vec4s){localOffsetX, localOffsetY, iconHW, iconHH};
                    cdBar->color     = (vec4s){0.1f, 0.0f, 0.0f, 1.0f};
                    cdBar->type      = QUADTYPE_FACECAM;
                    cdBar->textureId = SOL_TEXTURE_CLOUD2;
                    cdBar->uv        = (vec4s){0, 0, 1, 1};
                    cdBar->extra     = (vec4s){ 1.0f - (buff->ttl / buff->duration), 0, 0, 0};
                    cdBar->flags     = BITC(QUADFLAG_FILL_VERTICAL) | BITC(QUADFLAG_FILL_INVERT);
                }
            }
        }
    }
}
