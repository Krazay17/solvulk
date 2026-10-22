/*
 * File: prefabs.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-04
 *
 */
#include "sol/sol.h"

const ScCamera player_camera = {
    .fov              = 65.0f,
    .up.y             = 1.0f,
    .lerpspeed        = 10.0f,
    .desired_offset   = 1.0f,
    .desired_distance = 3.5f,
    // .y_offset         = 0.5f,
};

static const ScBody3 ball_body = {
    .shape       = SHAPE3_SPH,
    .mass        = 1.0f,
    .invMass     = 1.0f,
    .restitution = 0.5f,
    .gravity     = SOL_GRAVITY,
    .dims        = {0.5f},
    .mask        = PHYSXMASK(COLLAYER_ALL, COLLAYER_ALL),
    .base_mask   = PHYSXMASK(COLLAYER_ALL, COLLAYER_ALL),
};

static const ScBody3 wizard_body = {
    .shape       = SHAPE3_CAP,
    .mass        = 1.0f,
    .invMass     = 1.0f,
    .restitution = 0.2f,
    .gravity     = SOL_GRAVITY,
    .dims        = {0.5f, 1.5f, 0.5f},
    .mask        = PHYSXMASK(COLLAYER_PAWN, COLLAYER_ALL),
};

static const ScCombat wizard_combat = {
    .healthMax = 100.0f,
    .health    = 100.0f,
};

// static const ScInventory dude_inventory = {
//     .equipped_items = {0, 1, 2, 3, 4, 5, 6},
//     .items =
//         {
//             {.item_kind = ITEMKIND_CLAW, .uid = 1},
//             {.item_kind = ITEMKIND_CLAW, .uid = 2},
//             {.item_kind = ITEMKIND_CLAW, .uid = 3},
//             {.item_kind = ITEMKIND_CLAW, .uid = 4},

//             {.item_kind = ITEMKIND_CLAW, .uid = 5},
//             {.item_kind = ITEMKIND_CLAW, .uid = 6},
//             {.item_kind = ITEMKIND_CLAW, .uid = 7},
//         },
//     .item_count   = 7,
//     .next_item_id = 8,
// };

static const View3 healthbar = {
    .kind     = VIEW3KIND_POWERBAR,
    .color    = {0.1f, 0.9f, 0.1f, 1.0f},
    .duration = 4.0f,
    ._elapsed = 4.0f,
};

static const View3 energybar = {
    .kind     = VIEW3KIND_POWERBAR,
    .color    = {0.9f, 0.9f, 0.1f, 1.0f},
    .duration = 4.0f,
    ._elapsed = 4.0f,
};

static const ScAi wizard_ai = {
    .kind       = AIKIND_WIZARD,
    .aggroRange = 20.0f,
};

static const ScModel dude_model = {
    .kind = MODELKIND_DUDE,
};

static const ScBody3 dude_body = {
    .shape       = SHAPE3_CAP,
    .mass        = 1.0f,
    .invMass     = 1.0f,
    .restitution = 0.01f,
    .gravity     = SOL_GRAVITY,
    .dims        = {0.4f, 1.0f, 0.4f},
    .mask        = PHYSXMASK(COLLAYER_PAWN, COLLAYER_ALL),
    .base_mask   = PHYSXMASK(COLLAYER_PAWN, COLLAYER_ALL),
};

static const ScMove3 dude_move = {
    .kind       = MOVEMENTKIND_DUDE,
    .baseHeight = 1.8f,
};

static const ScCombat dude_combat = {
    .healthMax   = 100.0f,
    .health      = 100.0f,
    .energyMax   = 100.0f,
    .energy      = 100.0f,
    .respawnTime = 2.0f,
};

static const ScAnim anim_default = {
    .layers =
        {
            [0] = {.animId = 0, .currentAnim = 0, .blendFactor = 1.0f, .weight = 1.0f},
            [1] = {.animId = -1, .currentAnim = -1, .blendFactor = 1.0f, .weight = 1.0f},
            [2] = {.animId = -1, .currentAnim = -1, .blendFactor = 1.0f, .weight = 1.0f},
            [3] = {.animId = -1, .currentAnim = -1, .blendFactor = 1.0f, .weight = 1.0f},
        },
};

int Sol_Prefab_Dude(World *world, vec3s pos, float scale)
{
    int id = Sol_Create_Ent(world, pos);

    Xform_SetSca(world, id, glms_vec3_fill(1.1f));

    *Sol_Comp_Add(world, id, ScModel) = dude_model;
    *Sol_Comp_Add(world, id, ScAnim)  = anim_default;
    *Sol_Comp_Add(world, id, ScBody3) = dude_body;

    ScCombat *combat    = Sol_Comp_Add(world, id, ScCombat);
    *combat             = dude_combat;
    combat->respawnTime = 2.0f;

    Sol_Comp_Add(world, id, ScAbility);
    *Sol_Comp_Add(world, id, ScMove3)  = dude_move;
    *Sol_Comp_Add(world, id, ScCamera) = player_camera;

    ScInventory *inv = Sol_Comp_Add(world, id, ScInventory);
    Sol_Inv_Equip(world, id, 0, Sol_Inv_AddItem(world, id, &(SolItem){.item_kind = ITEMKIND_CLAW}, 0));
    Sol_Inv_Equip(world, id, 1, Sol_Inv_AddItem(world, id, &(SolItem){.item_kind = ITEMKIND_FIREBALL}, 0));
    Sol_Inv_Equip(world, id, 4, Sol_Inv_AddItem(world, id, &(SolItem){.item_kind = ITEMKIND_CLAW}, 0));
    Sol_Inv_Equip(world, id, 5, Sol_Inv_AddItem(world, id, &(SolItem){.item_kind = ITEMKIND_CLAW}, 0));
    Sol_Inv_Equip(world, id, 6, Sol_Inv_AddItem(world, id, &(SolItem){.item_kind = ITEMKIND_CLAW}, 0));

    Sol_Inv_AddItem(world, id, &(SolItem){.item_kind = ITEMKIND_SCYTHE}, 0);
    Sol_Inv_AddItem(world, id, &(SolItem){.item_kind = ITEMKIND_SCYTHE}, 0);
    Sol_Inv_AddItem(world, id, &(SolItem){.item_kind = ITEMKIND_SCYTHE}, 0);
    Sol_Inv_AddItem(world, id, &(SolItem){.item_kind = ITEMKIND_BOLT}, 0);
    Sol_Inv_AddItem(world, id, &(SolItem){.item_kind = ITEMKIND_BOLT}, 0);
    Sol_Inv_AddItem(world, id, &(SolItem){.item_kind = ITEMKIND_FIREBALL}, 0);

    ScView3 *view3 = Sol_Comp_Add(world, id, ScView3);
    solb_push(view3->views_b, healthbar);
    solb_push(view3->views_b, energybar);

    Sol_Comp_Add(world, id, ScTeam);
    Sol_Comp_Add(world, id, ScCmd);

    return id;
}

int Sol_Prefab_Wizard(World *world, vec3s pos, float scale)
{
    int id       = Sol_Create_Ent(world, pos);
    ScMeta *meta = Sol_Comp_Add(world, id, ScMeta);
    snprintf(meta->name, sizeof(meta->name), "Wizard %d", id);

    ScInteract *interact = Sol_Comp_Add(world, id, ScInteract);
    interact->range      = 25.0f;

    ScTeam *team = Sol_Comp_Add(world, id, ScTeam);
    team->team   = 1;

    Sol_Comp_Add(world, id, ScModel)->kind = MODELKIND_WIZARD;
    *Sol_Comp_Add(world, id, ScAnim)       = anim_default;

    ScView3 *view3 = Sol_Comp_Add(world, id, ScView3);
    solb_push(view3->views_b, healthbar);

    *Sol_Comp_Add(world, id, ScCombat) = wizard_combat;
    *Sol_Comp_Add(world, id, ScBody3)  = wizard_body;
    *Sol_Comp_Add(world, id, ScAi)     = wizard_ai;

    Sol_Comp_Add(world, id, ScAbility);

    Sol_Comp_Add(world, id, ScMove3)->kind = MOVEMENTKIND_WIZARD;
    Sol_Comp_Add(world, id, ScCmd);

    return id;
}

int Sol_Prefab_Crosshair(World *world)
{
    int id                = Sol_Create_Ent(world, (vec3s){(float)WINDOW_WIDTH / 2.0f, (float)WINDOW_HEIGHT / 2.0f, 0});
    ScView2 *sc_view  = Sol_Comp_Add(world, id, ScView2);
    View2 view0 = {
        .kind       = VIEW2KIND_RECT,
        .textureID  = SOL_TEXTURE_CROSSHAIR,
        .offset     = {-9.0f, -9.0f},
        .dims       = {18.0f, 18.0f},
        .color      = {1, 1, 1, 1},
        .scale      = 1.0f,
        .targetFill = 1.0f,
        .hoverColor = {1, 1, 1, 1},
        .textureUV  = {1, 1, 0, 0},
    };
    solb_push(sc_view->views, view0);

    return id;
}

int Sol_Prefab_Button(World *world, vec3s pos, const char *text, u32 interact_flags, u32 layer, Hook on_click)
{
    vec2s dims = {120.0f, 40.0f};

    int id = Sol_Create_Ent(world, pos);
    if (id < 0)
        return -1;
    ScInteract *interact = Sol_Comp_Add(world, id, ScInteract);
    interact->flags |= interact_flags;

    ScHook *hook  = Sol_Comp_Add(world, id, ScHook);
    hook->clicked = on_click;

    ScBody2 *body = Sol_Comp_Add(world, id, ScBody2);
    *body         = (ScBody2){
        .zindex = layer,
        .shape  = SHAPE2_REC,
        .dims   = {dims.x, dims.y, 0},
        .mask   = PHYSXMASK(COLLAYER_WORLD, COLLAYER_WORLD),
    };

    ScView2 *sc_view  = Sol_Comp_Add(world, id, ScView2);
    View2 view0 = {
        .layer       = layer,
        .kind        = VIEW2KIND_RECT,
        .dims        = {dims.x, dims.y},
        .color       = {0.0f, 0.0f, 0.0f, 1.0f},
        .hoverColor  = {1.0f, 1.0f, 1.0f, 1.0f},
        .activeColor = {0.0f, 1.0f, 0.0f, 1.0f},
        .downColor   = {0.0f, 0.0f, 0.0f, 1.0f},
    };
    View2 view1 = {
        .layer       = layer,
        .kind        = VIEW2KIND_RECT,
        .dims        = {dims.x, dims.y},
        .color       = {0.2f, 0.2f, 0.2f, 1.0f},
        .hoverColor  = {1.0f, 1.0f, 1.0f, 1.0f},
        .activeColor = {0.0f, 1.0f, 0.0f, 1.0f},
        .downColor   = {0.0f, 0.0f, 0.0f, 1.0f},
        .textureID   = SOL_TEXTURE_SWIRLFRAME,
    };
    View2 view2 = {
        .layer       = layer,
        .kind        = VIEW2KIND_RECT,
        .dims        = {dims.x, dims.y},
        .color       = {0.0f, 0.0f, 0.0f, 1.0f},
        .activeColor = {0.0f, 1.0f, 0.0f, 1.0f},
        .downColor   = {0.0f, 0.0f, 0.0f, 1.0f},
        .border      = 3.0f,
    };
    View2 view3 = {
        .layer       = layer,
        .kind        = VIEW2KIND_TEXT,
        .dims        = {14.0f},
        .color       = {0.0f, 1.0f, 0.0f, 1.0f},
        .activeColor = {0.0f, 1.0f, 0.0f, 1.0f},
        .downColor   = {0.0f, 0.0f, 0.0f, 1.0f},
        .offset      = {dims.x * 0.5f, dims.y * 0.5f},
    };
    strncpy(view3.text, text, sizeof(view3.text));
    solb_push(sc_view->views, view0);
    solb_push(sc_view->views, view1);
    solb_push(sc_view->views, view2);
    solb_push(sc_view->views, view3);

    return id;
}

static void Hook_PrintValue(World *w, int a, int b)
{
    sollog(Sol_Comp_Get(w, a, ScSlider)->value);
}
int Sol_Prefab_Slider(World *world, vec3s pos, const char *text, u32 interact_flags, u32 layer, Hook func)
{
    vec2s dims = {120.0f, 40.0f};

    int id = Sol_Create_Ent(world, pos);

    ScInteract *interact = Sol_Comp_Add(world, id, ScInteract);
    interact->flags |= interact_flags;

    ScBody2 body = {
        .zindex = layer,
        .shape  = SHAPE2_REC,
        .dims   = {dims.x, dims.y, 0},
        .mask   = PHYSXMASK(COLLAYER_WORLD, COLLAYER_WORLD),
    };
    *Sol_Comp_Add(world, id, ScBody2) = body;

    ScSlider slider = {
        .axis      = {1, 0, 0},
        .track_len = dims.x * 0.85f,
        .offset    = dims.x * 0.075f,
        .value     = 0.0f,
    };
    *Sol_Comp_Add(world, id, ScSlider) = slider;

    *Sol_Comp_Add(world, id, ScHook) = (ScHook){.held = func};

    ScView2 *sc_view  = Sol_Comp_Add(world, id, ScView2);
    View2 view0 = {
        .layer       = layer,
        .kind        = VIEW2KIND_RECT,
        .dims        = {dims.x, dims.y},
        .color       = {0.0f, 0.0f, 0.0f, 1.0f},
        .hoverColor  = {1.0f, 1.0f, 1.0f, 1.0f},
        .activeColor = {0.0f, 1.0f, 0.0f, 1.0f},
        .downColor   = {0.0f, 0.0f, 0.0f, 1.0f},
    };
    View2 view1 = {
        .layer = layer,
        .kind  = VIEW2KIND_SLIDER_FILL,
        .dims  = {dims.x, dims.y},
        .color = {0.1f, 0.1f, 0.7f, 1.0f},
    };
    View2 view2 = {
        .layer       = layer,
        .kind        = VIEW2KIND_RECT,
        .dims        = {dims.x, dims.y},
        .color       = {0.2f, 0.2f, 0.2f, 1.0f},
        .hoverColor  = {1.0f, 1.0f, 1.0f, 1.0f},
        .activeColor = {0.0f, 1.0f, 0.0f, 1.0f},
        .downColor   = {0.0f, 0.0f, 0.0f, 1.0f},
        .textureID   = SOL_TEXTURE_SWIRLFRAME,
    };
    View2 view3 = {
        .layer       = layer,
        .kind        = VIEW2KIND_RECT,
        .dims        = {dims.x, dims.y},
        .color       = {0.0f, 0.0f, 0.0f, 1.0f},
        .activeColor = {0.0f, 1.0f, 0.0f, 1.0f},
        .downColor   = {0.0f, 0.0f, 0.0f, 1.0f},
        .border      = 3.0f,
    };

    View2 view4 = {
        .layer = layer,
        .kind  = VIEW2KIND_SLIDER,
        .dims  = {dims.x, dims.y},
        .color = {0.1f, 0.1f, 0.7f, 1.0f},
    };
    View2 view5 = {
        .layer     = layer,
        .kind      = VIEW2KIND_SLIDER,
        .dims      = {dims.x, dims.y},
        .color     = {0.5f, 0.1f, 0.7f, 1.0f},
        .textureID = SOL_TEXTURE_SWIRLFRAME,
    };
    View2 view6 = {
        .layer  = layer,
        .kind   = VIEW2KIND_SLIDER,
        .dims   = {dims.x, dims.y},
        .color  = {0.0f, 0.0f, 0.0f, 1.0f},
        .border = 3.0f,
    };
    View2 view7 = {
        .layer       = layer,
        .kind        = VIEW2KIND_TEXT,
        .dims        = {14.0f},
        .color       = {0.0f, 1.0f, 0.0f, 1.0f},
        .activeColor = {0.0f, 1.0f, 0.0f, 1.0f},
        .downColor   = {0.0f, 0.0f, 0.0f, 1.0f},
        .offset      = {dims.x * 0.5f, dims.y * 0.5f},
    };
    strncpy(view7.text, text, sizeof(view7.text));

    solb_push(sc_view->views, view0);
    solb_push(sc_view->views, view1);
    solb_push(sc_view->views, view2);
    solb_push(sc_view->views, view3);
    solb_push(sc_view->views, view4);
    solb_push(sc_view->views, view5);
    solb_push(sc_view->views, view6);
    solb_push(sc_view->views, view7);


    return id;
}

int Sol_Prefab_Powerbar(World *world, vec3s pos, vec4s color, float dim_y)
{
    vec2s dims = {300.0f, dim_y};

    int id               = Sol_Create_Ent(world, pos);
    ScInteract *interact = Sol_Comp_Add(world, id, ScInteract);
    interact->flags |= INTERACTFLAG_ONLYDRAGGABLE;

    ScBody2 *body = Sol_Comp_Add(world, id, ScBody2);
    *body         = (ScBody2){
        .shape       = SHAPE2_REC,
        .restitution = 1.0f,
        .dims        = {dims.x, dims.y, 0},
        .mask        = PHYSXMASK(COLLAYER_ALL, COLLAYER_ALL),
    };

    ScView2 *sc_view  = Sol_Comp_Add(world, id, ScView2);
    View2 view0 = {
        .kind       = VIEW2KIND_RECT,
        .dims       = {dims.x, dims.y},
        .color      = {0.0f, 0.0f, 0.0f, 1.0f},
        .hoverColor = {1, 1, 1, 0.5f},
    };
    View2 view1 = {
        .kind       = VIEW2KIND_RECT,
        .dims       = {dims.x, dims.y},
        .color      = {0.2f, 0.2f, 0.2f, 1.0f},
        .hoverColor = {1, 1, 1, 0.5f},
        .textureID  = SOL_TEXTURE_HEALTH,
    };
    View2 view2 = {
        .kind       = VIEW2KIND_POWERBAR,
        .dims       = {dims.x, dims.y},
        .color      = {1.0f, 0.0f, 0.0f, 1.0f},
        .fillSpeed  = 1.5f,
        .hoverColor = {1, 1, 0, 0.5f},
        .textureID  = SOL_TEXTURE_HEALTH,
    };
    View2 view3 = {
        .kind       = VIEW2KIND_POWERBAR,
        .dims       = {dims.x, dims.y},
        .color      = color,
        .fillSpeed  = 10.0f,
        .hoverColor = {1, 1, 0, 0.5f},
        .textureID  = SOL_TEXTURE_HEALTH,
    };
    View2 view4 = {
        .kind   = VIEW2KIND_RECT,
        .dims   = {dims.x, dims.y},
        .color  = {0.0f, 0.0f, 0.0f, 1.0f},
        .border = 2.0f,
    };
    View2 view5 = {
        .layer  = UILAYER_3,
        .kind   = VIEW2KIND_TEXT,
        .dims   = {dims.y * 0.5f},
        .offset = {dims.x * 0.5f, dims.y * 0.5f},
        .color  = {1.0f, 0.0f, 0.0f, 1.0f},
    };
    solb_push(sc_view->views, view0);
    solb_push(sc_view->views, view1);
    solb_push(sc_view->views, view2);
    solb_push(sc_view->views, view3);
    solb_push(sc_view->views, view4);
    solb_push(sc_view->views, view5);

    return id;
}

int Sol_Prefab_Fireball(World *world, int owner, vec3s pos, vec3s dir, float speed, float size)
{
    int id = Sol_Create_Ent(world, pos);

    vec3s dims          = {size, size, size};
    ScBody3 *owner_body = Sol_Comp_Get(world, owner, ScBody3);

    *Sol_Comp_Add(world, id, ScOwner) = (ScOwner){
        .ownerId = owner,
    };

    *Sol_Comp_Add(world, id, ScBody3) = (ScBody3){
        .dims      = dims,
        .gravity   = SOL_GRAVITY,
        .is_sensor = true,
        .ignoreEnt = owner,
        .vel       = vecSca(dir, speed),
        .mask      = PHYSXMASK(COLLAYER_PROJECTILE, 0),
    };

    ScView3 *view3 = Sol_Comp_Add(world, id, ScView3);
    View3 fireball = {
        .kind  = VIEW3KIND_FIREBALL,
        .color = VEC4_RED,
        .scale = size,
    };
    solb_push(view3->views_b, fireball);

    *Sol_Comp_Add(world, id, ScProjectile) = (ScProjectile){
        .kind          = PROJECTILEKIND_FIREBALL,
        .radius        = size * 0.33f,
        .hitgen        = Sol_Hitgen_Start(world),
        .target_radius = size,
    };

    ScAi *ai = Sol_Comp_Get(world, owner, ScAi);
    if (ai)
    {
        ScAilearn *ailearn             = Sol_Comp_Add(world, id, ScAilearn);
        ailearn->prev_knows_move.raw   = ai->learning.prev_knows_move.raw;
        ailearn->prev_knows_combat.raw = ai->learning.prev_knows_combat.raw;
        ailearn->action_move           = ai->learning.action_move;
        ailearn->action_combat         = ai->learning.action_combat;
        ailearn->reward_move           = 5.0f;
        ailearn->reward_combat         = 10.0f;
    }

    Ribbon *r       = Sol_Ribbon_AddKind(world, id, RIBBONKIND_FIRE);
    r->rate         = 0.02f;
    r->thickness[0] = size;
    Sol_Ribbon_Addpoint(r, pos, WORLD_UP);

    return id;
}

int Sol_Prefab_Crystal(World *world, vec3s pos)
{
    return 0;
}

int Sol_Prefab_Inventory(World *world, vec3s pos, const ScInventory *inventory)
{
    int id     = Sol_Create_Ent(world, pos);
    vec2s dims = {200.0f, 200.0f};

    *Sol_Comp_Add(world, id, ScBody2) = (ScBody2){
        .zindex   = UILAYER_0,
        .shape    = SHAPE2_REC,
        .dims     = (vec3s){dims.x, dims.y, 0},
        .isSensor = true,
        .mask     = PHYSXMASK(COLLAYER_ALL, COLLAYER_ALL),
    };

    float offset = 0.0f;
    for (int i = 0; i < ITEMKIND_COUNT; i++)
    {
        // if (inventory->unlocked_items[i])
        // {
        //     int card_id              = Sol_Prefab_AbilityCard(world, pos, &(SolItem){.item_kind = i});
        //     ScParent *card_parent    = Sol_Comp_Add(world, card_id, ScParent);
        //     card_parent->parentId    = id;
        //     card_parent->localOffset = (vec3s){offset, 0, 0};
        //     offset += 64.0f;
        // }
    }

    ScInteract *interact = Sol_Comp_Add(world, id, ScInteract);
    interact->flags      = INTERACTFLAG_DRAGGABLE;

    ScView2 *sc_view  = Sol_Comp_Add(world, id, ScView2);
    View2 view0 = {.kind = VIEW2KIND_INVENTORY};
    solb_push(sc_view->views, view0);

    return id;
}

int Sol_Prefab_AbilityBar(World *world, vec3s pos, int ent_id, int ent_world_idx, const int *slots, int slot_count)
{
    int id = Sol_Create_Ent(world, pos);

    vec2s slot_dims = {66.0f, 66.0f};
    vec2s dims      = {slot_dims.x * slot_count, slot_dims.y};

    *Sol_Comp_Add(world, id, ScBody2) = (ScBody2){
        .zindex   = UILAYER_0,
        .shape    = SHAPE2_REC,
        .dims     = (vec3s){dims.x, dims.y, 0},
        .isSensor = true,
        .mask     = PHYSXMASK(COLLAYER_ALL, COLLAYER_ALL),
    };

    ScInteract *interact = Sol_Comp_Add(world, id, ScInteract);
    interact->flags      = INTERACTFLAG_ONLYDRAGGABLE;

    *Sol_Comp_Add(world, id, ScRef) = (ScRef){.kind = REFKIND_ABILITYBAR, .world = ent_world_idx, .id = ent_id};

    ScAbilitybar *ability_bar = Sol_Comp_Add(world, id, ScAbilitybar);
    ability_bar->slot_dims    = slot_dims;
    ability_bar->slot_count   = slot_count;
    memcpy(ability_bar->ability_slots, slots, sizeof(slots[0]) * slot_count);

    View2 bg        = {.dims = dims, .color = VEC4_BLACK, .hoverColor = VEC4_WHITE};
    View2 slot_view = {.kind = VIEW2KIND_ABILITYBAR, .dims = dims, .color = VEC4_BLACK, .layer = UILAYER_2};
    ScView2 *view2  = Sol_Comp_Add(world, id, ScView2);
    solb_push(view2->views, bg);
    solb_push(view2->views, slot_view);

    return id;
}

int Sol_Prefab_AbilityCard(World *world, vec3s pos, SolItem *item)
{
    vec2s dims  = {62.0f, 62.0f};
    u32 texture = item_texture_map[item->item_kind];
    u32 layer   = UILAYER_1;

    int id = Sol_Create_Ent(world, pos);

    ScInteract *interact = Sol_Comp_Add(world, id, ScInteract);
    interact->flags      = INTERACTFLAG_ONLYDRAGGABLE;

    *Sol_Comp_Add(world, id, ScBody2) = (ScBody2){
        .zindex   = layer,
        .shape    = SHAPE2_REC,
        .dims.x   = dims.x,
        .dims.y   = dims.y,
        .isSensor = true,
        .mask     = PHYSXMASK(COLLAYER_ALL, COLLAYER_ALL),
    };

    *Sol_Comp_Add(world, id, ScTooltip) = (ScTooltip){
        .kind = TOOLTIPKIND_CARD,
    };

    ScView2 *sc_view = Sol_Comp_Add(world, id, ScView2);
    View2 view       = {
        .layer       = layer,
        .kind        = VIEW2KIND_RECT,
        .dims        = {dims.x, dims.y},
        .textureID   = texture,
        .textureUV   = {1.0f, 0.816f, 0, 0},
        .flags       = 1,
        .targetFill  = 1.0f,
        .color       = {1, 1, 1, 1},
        .hoverColor  = {0.5f, 0.5f, 0.5f, 1.0f},
        .activeColor = {1, 1, 1, 1},
        .downColor   = {1, 1, 1, 1},
    };
    View2 view2 = {
        .layer       = layer,
        .kind        = VIEW2KIND_RECT,
        .dims        = {dims.x, dims.y},
        .textureID   = SOL_TEXTURE_BORDER,
        .color       = rarity_color[item->rarity],
        .hoverColor  = {1.0f, 1.0f, 1.0f, 1.0f},
        .activeColor = {1, 1, 1, 1},
        .downColor   = {1, 1, 1, 1},
    };
    solb_push(sc_view->views, view);
    solb_push(sc_view->views, view2);
    return id;
}

int Sol_Prefab_LightningBolt(World *world, int owner, vec3s pos, vec3s dir, float speed, float scale, Hook hook)
{
    int id = Sol_Create_Ent(world, pos);

    vec3s dims = {scale, scale, scale};
    u32 mask   = PHYSXMASK(COLLAYER_PROJECTILE, (COLLAYER_ALL & ~COLLAYER_PROJECTILE));

    Xform_SetRot(world, id, glms_quat_from_vecs((vec3s){0, 1, 0}, dir));
    *Sol_Comp_Add(world, id, ScOwner) = (ScOwner){
        .ownerId = owner,
    };
    *Sol_Comp_Add(world, id, ScBody3) = (ScBody3){
        .dims      = dims,
        .mask      = mask,
        .mass      = 1.0f,
        .vel       = glms_vec3_scale(dir, speed),
        .shape     = SHAPE3_SPH,
        .is_sensor = 1,
    };
    *Sol_Comp_Add(world, id, ScProjectile) = (ScProjectile){
        .kind   = PROJECTILEKIND_LIGHTNINGBOLT,
        .radius = dims.x,
        .mask   = mask,
        .hook   = hook,
        .hitgen = Sol_Hitgen_Start(world),
    };
    ScView3 *view3 = Sol_Comp_Add(world, id, ScView3);
    View3 bolt     = {
        .kind  = VIEW3KIND_BOLT,
        .scale = dims.x,
        .color = VEC4_WHITE,
    };
    solb_push(view3->views_b, bolt);

    Ribbon *ribbon = Sol_Ribbon_AddKind(world, id, RIBBONKIND_LIGHTNING);
    ribbon->rate   = 0.033f;
    ribbon->sheets = 2;
    ribbon->flags  = RIBBONFLAG_NOFACECAM;
    Sol_Ribbon_Addpoint(ribbon, pos, WORLD_UP);

    return id;
}

int Sol_Prefab_PowerVial(World *world, vec3s pos, vec4s color)
{
    vec2s dims = {120.0f, 120.0f};

    int id = Sol_Create_Ent(world, pos);

    ScInteract *interact = Sol_Comp_Add(world, id, ScInteract);
    interact->flags |= INTERACTFLAG_DRAGGABLE;

    ScBody2 *body = Sol_Comp_Add(world, id, ScBody2);
    *body         = (ScBody2){
        .shape       = SHAPE2_REC,
        .restitution = 1.0f,
        .dims        = {dims.x, dims.y, 0},
        .mask        = PHYSXMASK(COLLAYER_ALL, COLLAYER_ALL),
    };

    ScView2 *sc_view = Sol_Comp_Add(world, id, ScView2);
    View2 fill     = {
        .kind      = VIEW2KIND_POWERBAR,
        .color     = color,
        .dims      = dims,
        .textureID = Sol_TEXTURE_HEALTHVIALFILL,
        .fillSpeed = 5.0f,
        .flags     = 0b111,
    };
    View2 frame = {.kind = VIEW2KIND_RECT, .color = color, .dims = dims, .textureID = Sol_TEXTURE_HEALTHVIAL};
    solb_push(sc_view->views, fill);
    solb_push(sc_view->views, frame);

    return id;
}

int Sol_Prefab_InventoryWindow(World *world, vec3s pos)
{
    int id = Sol_Create_Ent(world, pos);

    vec2s dims = {200.0f, 200.0f};

    *Sol_Comp_Add(world, id, ScBody2) = (ScBody2){
        .zindex   = UILAYER_1,
        .shape    = SHAPE2_REC,
        .dims     = (vec3s){dims.x, dims.y, 0},
        .isSensor = true,
        .mask     = PHYSXMASK(COLLAYER_ALL, COLLAYER_ALL),
    };

    ScInteract *interact = Sol_Comp_Add(world, id, ScInteract);
    interact->flags |= INTERACTFLAG_DRAGGABLE;

    ScView2 *sc_view = Sol_Comp_Add(world, id, ScView2);
    View2 view     = {.kind = VIEW2KIND_INVENTORY};
    solb_push(sc_view->views, view);

    return id;
}

int Sol_Prefab_RulesWindow(World *world, vec3s pos, u32 view_count)
{
    int id = Sol_Create_Ent(world, pos);

    vec2s dims = {220.0f, 200.0f};

    *Sol_Comp_Add(world, id, ScBody2) = (ScBody2){
        .zindex   = UILAYER_1,
        .shape    = SHAPE2_REC,
        .dims     = (vec3s){dims.x, dims.y, 0},
        .isSensor = true,
        .mask     = PHYSXMASK(COLLAYER_ALL, COLLAYER_ALL),
    };

    ScInteract *interact = Sol_Comp_Add(world, id, ScInteract);
    interact->flags |= INTERACTFLAG_DRAGGABLE;

    const float text_size = 16.0f;
    ScView2 *sc_view        = Sol_Comp_Add(world, id, ScView2);
    View2 view0 = {.kind = VIEW2KIND_RULESWINDOW};
    solb_push(sc_view->views, view0);
    float y_offset_start  = 10.0f;
    for (int i = 1; i < view_count; i++)
    {
        View2 view = {.kind      = VIEW2KIND_RULES_TEXT,
                      .textWidth = 16.0f,
                      .dims      = {text_size},
                      .offset    = {8.0f, y_offset_start + (text_size * i)},
                      .color     = VEC4_GREEN};
        solb_push(sc_view->views, view);
    }

    return id;
}

int Sol_Prefab_GravPad(World *world, vec3s pos, versors rot)
{
    int id = Sol_Create_Ent(world, pos);

    ScModel *model = Sol_Comp_Add(world, id, ScModel);
    ScBody3 *body3 = Sol_Comp_Add(world, id, ScBody3);
    body3->dims    = (vec3s){1.0f, 0.3f, 4.0f};
    body3->shape   = SHAPE3_CYL;
    model->kind    = MODELKIND_JUMPPAD;
    Sol_Comp_Add(world, id, ScAnim);
}

int Sol_Prefab_Pickup(World *world, vec3s pos)
{
    float size       = 0.5f;
    int id           = Sol_Create_Ent(world, pos);
    ScPickup *pickup = Sol_Comp_Add(world, id, ScPickup);
    pickup->item     = (SolItem){.item_kind = ITEMKIND_FIREBALL, .effectMask = EFFECTMASK_KNOCKUP};
    pickup->range    = size;

    ScView3 *view3 = Sol_Comp_Add(world, id, ScView3);
    View3 sphere   = {.color = VEC4_WHITE, .kind = VIEW3KIND_SPHERE, .scale = size};
    solb_push(view3->views_b, sphere);

    *Sol_Comp_Add(world, id, ScBody3) = ball_body;

    return id;
}