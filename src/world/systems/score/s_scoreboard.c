#include "world.h"
#include "sol_user.h"
#include "render/render.h"

const float yOffset = 100.0f;
const float xOffset = 200.0f;

void Scoreboard_Draw(World *world, double dt)
{
    if (!(sol_user.actions & BITC(ACTION_SCORE)))
        return;
    // SparseSet_ScCombat *set = Sol_Comp_Set(world, ScCombat);
    SparseSet_ScMeta *set = Sol_Comp_Set(world, ScMeta);

    for (int i = 0; i < set->cnt; i++)
    {
        int id       = set->dense[i];
        ScMeta *meta = &set->data[i];

        Sol_Render_DrawText2D(meta->name, (SolFontDesc){
                                              .x     = xOffset,
                                              .y     = i * 20.0f + yOffset,
                                              .color = VEC4_GREEN,
                                              .layer = UILAYER_1,
                                              .size  = 16.0f,
                                          });
        if (Sol_Comp_Has(world, id, ScCombat))
        {
            ScCombat *combat = Sol_Comp_Get(world, id, ScCombat);

            SolFontDesc font_combat = {
                .x     = xOffset + 150.0f,
                .y     = i * 18.0f + yOffset,
                .color = VEC4_GREEN,
                .layer = UILAYER_1,
                .size  = 16.0f,
            };
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "Damage %.2f", combat->damageDone);
            Sol_Render_DrawText2D(buffer, font_combat);
        }
    }
}