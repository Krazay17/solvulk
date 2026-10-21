#include "world.h"
#include "sol_user.h"
#include "render/render.h"

const float yOffset = 100.0f;
const float xOffset = 200.0f;

void Scoreboard_Draw(World *world, double dt)
{
    if (!(sol_user.actions & BITC(ACTION_SCORE)))
        return;
    SparseSet_ScMeta *set   = Sol_Comp_Set(world, ScMeta);
    SolFontDesc font_combat = {
        .x     = xOffset,
        .color = VEC4_GREEN,
        .layer = UILAYER_1,
        .size  = 16.0f,
    };

    for (int i = 0; i < set->cnt; i++)
    {
        int id       = set->dense[i];
        ScMeta *meta = &set->data[i];
        u32 kills    = 0;
        float damage = 0;
        forc(world, id, ScScore)
        {
            kills  = c->kills;
            damage = c->damage;
        }
        font_combat.y = i * 18.0f + yOffset;
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "%-16s - ID:%3d - Kills: %3u - Damage: %.2f", meta->name, id, kills, damage);
        Sol_Render_DrawText2D(buffer, font_combat);
    }
}