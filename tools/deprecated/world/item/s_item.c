/*
 * File: s_item.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-06-28
 * Example of how systems could operate under void *components
 */

#include "s_item.h"
#include "sol_core.h"
#include "sol_core.h"
#include "sol_math.h"
#include "world.h"
#include "xform/s_xform.h"
#include "physx/s_body2d.h"
#include "game/prefabs.h"

#include "player/s_player.h"
#include "controller/s_controller.h"
#include "ability/s_ability.h"
#include "parent/s_parent.h"
#include "interact/s_interact.h"
#include "view/s_view2d.h"
#include "render/render.h"


// static void Draw2d(World *world, double dt, double time)
// {
//     static int required = BITC(HAS_BODY2) | BITC(HAS_ITEM);
//     int        counter  = 0;
//     for (int i = 0; i < world->activeCount; i++)
//     {
//         int id = world->activeEntities[i];
//         if (!WHas(world, id, required))
//             continue;
//         CompItem   *item  = &world->items[id];
//         CompXform  *xform = &world->xforms[id];
//         CompBody2d *body  = &world->body2d[id];

//         RectSSBO *rect  = Sol_Render_GetNext_Rect();
//         rect->rect      = (vec4s){xform->drawPos.x, xform->drawPos.y, body->dims.x, body->dims.y};
//         rect->scale     = 1.0f;
//         rect->fill      = 1.0f;
//         rect->color     = (vec4s){1, 1, 1, 1};
//         rect->uv        = (vec4s){0, 0, 1, 1};
//         rect->textureID = ability_icon[item->item.ability.state];
//         counter++;
//     }
// }

void Sol_Item_Init(World *world)
{
    // world->items        = calloc(MAX_ENTS, sizeof(CompItem));
    // world->abilitySlots = calloc(MAX_ENTS, sizeof(CompAbilitySlot));

    // WAddStep(world) = AbilitySlots;
    // WAdd2d(world) = Draw2d;
}

void Sol_Item_SetRarity(World *world, int id, u32 rarity)
{
    // rarity                    = min(3, rarity);
    // CompItem    *item         = &world->items[id];
    // CompTooltip *tooltip      = &world->tooltips[id];
    // CompView2d  *view         = &world->view2d[id];
    // item->item.ability.rarity = rarity;
    // vec4s color;
    // switch (rarity)
    // {
    // case 0:
    //     color = (vec4s){0.5f, 0.5f, 0.5f, 1.0f};
    //     break;
    // case 1:
    //     color = (vec4s){0.0f, 1.0f, 0.0f, 1.0f};
    //     break;
    // case 2:
    //     color = (vec4s){1.0f, 0.0f, 1.0f, 1.0f};
    //     break;
    // case 3:
    //     color = (vec4s){1.0f, 0.2f, 0.0f, 1.0f};
    //     break;
    // }
    // view->views[1].color      = color;
    // view->views[1].hoverColor = color;
}

void Sol_Item_Drop(World *world, int id)
{
    // Sol_Prefab_ItemDrop(world, Sol_Xform_GetPos(world, id));
}