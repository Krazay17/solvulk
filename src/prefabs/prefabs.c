/*
 * File: prefabs.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-04
 *
 */
#include "sol/sol.h"

const ScCamera player_camera = {
    .fov              = 60.0f,
    .up.y             = 1.0f,
    .lerpspeed        = 10.0f,
    .desired_offset   = 0.5f,
    .desired_distance = 5.0f,
    .y_offset         = 0.5f,
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

static const ScInventory dude_inventory = {
    .equipped_items = {0, 1, 2, 3, 4, 5, 6},
    .items =
        {
            {.item_kind = ITEMKIND_CLAW},
            {.item_kind = ITEMKIND_CLAW},
            {.item_kind = ITEMKIND_CLAW},
            {.item_kind = ITEMKIND_CLAW},

            {.item_kind = ITEMKIND_CLAW},
            {.item_kind = ITEMKIND_CLAW},
            {.item_kind = ITEMKIND_CLAW},
        },
    .item_count = 7,
};

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
    .dims        = {0.35f, 1.0f, 0.35f},
    .mask        = PHYSXMASK(COLLAYER_PAWN, COLLAYER_ALL),
};

static const ScBody3 sphere_body = {
    .shape       = SHAPE3_SPH,
    .mass        = 1.0f,
    .invMass     = 1.0f,
    .restitution = 0.5f,
    .gravity     = SOL_GRAVITY,
    .dims        = {1.0f, 1.0f, 1.0f},
    .mask        = PHYSXMASK(COLLAYER_ALL, COLLAYER_ALL),
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

static const ScAbility dude_ability = {
    .base_actions =
        {
            ITEMKIND_CRYSTAL,
            ITEMKIND_FIREBALL,
            ITEMKIND_CLAW,
            0,
            ITEMKIND_SCYTHE,
            ITEMKIND_SCYTHE,
            ITEMKIND_SCYTHE,
        },
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

    *Sol_Comp_Add(world, id, ScAbility)   = dude_ability;
    *Sol_Comp_Add(world, id, ScMove3)     = dude_move;
    *Sol_Comp_Add(world, id, ScCamera)    = player_camera;
    
    ScInventory *inv = Sol_Comp_Add(world, id, ScInventory);
    *inv = dude_inventory;
    inv->dirty = 1;
    for(int i = 0;i<inv->item_count;i++)
    {
        int uid = ++inv->next_item_id;
        inv->items[i].uid = uid;
    }

    ScView3 *view3 = Sol_Comp_Add(world, id, ScView3);
    solb_push(view3->views_b, healthbar);
    solb_push(view3->views_b, energybar);

    Sol_Comp_Add(world, id, ScTeam);
    Sol_Comp_Add(world, id, ScCmd);

    for (int i = 0; i < ABILITY_SLOTS; i++)
        Sol_Equip_Item(world, id, ITEMKIND_CLAW, i);

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

    *Sol_Comp_Add(world, id, ScAbility) = (ScAbility){.base_actions = {
                                                          ITEMKIND_CRYSTAL,
                                                          ITEMKIND_CRYSTAL,
                                                          ITEMKIND_CRYSTAL,
                                                          ITEMKIND_CRYSTAL,
                                                          ITEMKIND_CRYSTAL,
                                                          ITEMKIND_FIREBALL,
                                                          ITEMKIND_FIREBALL,
                                                      }};

    Sol_Comp_Add(world, id, ScMove3)->kind = MOVEMENTKIND_WIZARD;
    Sol_Comp_Add(world, id, ScCmd);

    return id;
}

int Sol_Prefab_Crosshair(World *world)
{
    int id                = Sol_Create_Ent(world, (vec3s){(float)WINDOW_WIDTH / 2.0f, (float)WINDOW_HEIGHT / 2.0f, 0});
    ScView2 *buttonView2  = Sol_Comp_Add(world, id, ScView2);
    buttonView2->count    = 1;
    buttonView2->views[0] = (View2){
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

    ScView2 *view  = Sol_Comp_Add(world, id, ScView2);
    view->count    = 4;
    view->views[0] = (View2){
        .layer       = layer,
        .kind        = VIEW2KIND_RECT,
        .dims        = {dims.x, dims.y},
        .color       = {0.1f, 0.1f, 0.1f, 1.0f},
        .hoverColor  = {1.0f, 1.0f, 1.0f, 1.0f},
        .activeColor = {0.0f, 1.0f, 0.0f, 1.0f},
        .downColor   = {0.0f, 0.0f, 0.0f, 1.0f},
    };
    view->views[1] = (View2){
        .layer       = layer,
        .kind        = VIEW2KIND_RECT,
        .dims        = {dims.x, dims.y},
        .color       = {0.9f, 0.1f, 0.1f, 1.0f},
        .hoverColor  = {1.0f, 1.0f, 1.0f, 1.0f},
        .activeColor = {0.0f, 1.0f, 0.0f, 1.0f},
        .downColor   = {0.0f, 0.0f, 0.0f, 1.0f},
        .textureID   = SOL_TEXTURE_SWIRLFRAME,
    };
    view->views[2] = (View2){
        .layer       = layer,
        .kind        = VIEW2KIND_RECT,
        .dims        = {dims.x, dims.y},
        .color       = {0.0f, 0.0f, 0.0f, 1.0f},
        .activeColor = {0.0f, 1.0f, 0.0f, 1.0f},
        .downColor   = {0.0f, 0.0f, 0.0f, 1.0f},
        .border      = 3.0f,
    };
    view->views[3] = (View2){
        .layer       = layer,
        .kind        = VIEW2KIND_TEXT,
        .dims        = {14.0f},
        .color       = {0.0f, 1.0f, 0.0f, 1.0f},
        .activeColor = {0.0f, 1.0f, 0.0f, 1.0f},
        .downColor   = {0.0f, 0.0f, 0.0f, 1.0f},
        .offset      = {dims.x * 0.5f, dims.y * 0.5f},
    };
    strncpy(view->views[3].text, text, sizeof(view->views[3].text));

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

    ScView2 *view  = Sol_Comp_Add(world, id, ScView2);
    view->count    = 8;
    view->views[0] = (View2){
        .layer       = layer,
        .kind        = VIEW2KIND_RECT,
        .dims        = {dims.x, dims.y},
        .color       = {0.1f, 0.1f, 0.1f, 1.0f},
        .hoverColor  = {1.0f, 1.0f, 1.0f, 1.0f},
        .activeColor = {0.0f, 1.0f, 0.0f, 1.0f},
        .downColor   = {0.0f, 0.0f, 0.0f, 1.0f},
    };
    view->views[1] = (View2){
        .layer = layer,
        .kind  = VIEW2KIND_SLIDER_FILL,
        .dims  = {dims.x, dims.y},
        .color = {0.1f, 0.1f, 0.7f, 1.0f},
    };
    view->views[2] = (View2){
        .layer       = layer,
        .kind        = VIEW2KIND_RECT,
        .dims        = {dims.x, dims.y},
        .color       = {0.9f, 0.1f, 0.1f, 1.0f},
        .hoverColor  = {1.0f, 1.0f, 1.0f, 1.0f},
        .activeColor = {0.0f, 1.0f, 0.0f, 1.0f},
        .downColor   = {0.0f, 0.0f, 0.0f, 1.0f},
        .textureID   = SOL_TEXTURE_SWIRLFRAME,
    };
    view->views[3] = (View2){
        .layer       = layer,
        .kind        = VIEW2KIND_RECT,
        .dims        = {dims.x, dims.y},
        .color       = {0.0f, 0.0f, 0.0f, 1.0f},
        .activeColor = {0.0f, 1.0f, 0.0f, 1.0f},
        .downColor   = {0.0f, 0.0f, 0.0f, 1.0f},
        .border      = 3.0f,
    };

    view->views[4] = (View2){
        .layer = layer,
        .kind  = VIEW2KIND_SLIDER,
        .dims  = {dims.x, dims.y},
        .color = {0.1f, 0.1f, 0.7f, 1.0f},
    };
    view->views[5] = (View2){
        .layer     = layer,
        .kind      = VIEW2KIND_SLIDER,
        .dims      = {dims.x, dims.y},
        .color     = {0.5f, 0.1f, 0.7f, 1.0f},
        .textureID = SOL_TEXTURE_SWIRLFRAME,
    };
    view->views[6] = (View2){
        .layer  = layer,
        .kind   = VIEW2KIND_SLIDER,
        .dims   = {dims.x, dims.y},
        .color  = {0.0f, 0.0f, 0.0f, 1.0f},
        .border = 3.0f,
    };
    view->views[7] = (View2){
        .layer       = layer,
        .kind        = VIEW2KIND_TEXT,
        .dims        = {14.0f},
        .color       = {0.0f, 1.0f, 0.0f, 1.0f},
        .activeColor = {0.0f, 1.0f, 0.0f, 1.0f},
        .downColor   = {0.0f, 0.0f, 0.0f, 1.0f},
        .offset      = {dims.x * 0.5f, dims.y * 0.5f},
    };
    strncpy(view->views[7].text, text, sizeof(view->views[7].text));

    return id;
}

int Sol_Prefab_Powerbar(World *world, vec3s pos, vec4s color, float dim_y)
{
    vec2s dims = {300.0f, dim_y};

    int id               = Sol_Create_Ent(world, pos);
    ScInteract *interact = Sol_Comp_Add(world, id, ScInteract);
    interact->flags |= INTERACTFLAG_DRAGGABLE;

    ScBody2 *body = Sol_Comp_Add(world, id, ScBody2);
    *body         = (ScBody2){
        .shape       = SHAPE2_REC,
        .restitution = 1.0f,
        .dims        = {dims.x, dims.y, 0},
        .mask        = PHYSXMASK(COLLAYER_ALL, COLLAYER_ALL),
    };

    ScView2 *view  = Sol_Comp_Add(world, id, ScView2);
    view->count    = 6;
    view->views[0] = (View2){
        .kind       = VIEW2KIND_RECT,
        .dims       = {dims.x, dims.y},
        .color      = {0.0f, 0.0f, 0.0f, 1.0f},
        .hoverColor = {1, 1, 1, 0.5f},
    };
    view->views[1] = (View2){
        .kind       = VIEW2KIND_RECT,
        .dims       = {dims.x, dims.y},
        .color      = {0.2f, 0.2f, 0.2f, 1.0f},
        .hoverColor = {1, 1, 1, 0.5f},
        .textureID  = SOL_TEXTURE_HEALTH,
    };
    view->views[2] = (View2){
        .kind       = VIEW2KIND_POWERBAR,
        .dims       = {dims.x, dims.y},
        .color      = {1.0f, 0.0f, 0.0f, 1.0f},
        .fillSpeed  = 1.5f,
        .hoverColor = {1, 1, 0, 0.5f},
        .textureID  = SOL_TEXTURE_HEALTH,
    };
    view->views[3] = (View2){
        .kind       = VIEW2KIND_POWERBAR,
        .dims       = {dims.x, dims.y},
        .color      = color,
        .fillSpeed  = 10.0f,
        .hoverColor = {1, 1, 0, 0.5f},
        .textureID  = SOL_TEXTURE_HEALTH,
    };
    view->views[4] = (View2){
        .kind   = VIEW2KIND_RECT,
        .dims   = {dims.x, dims.y},
        .color  = {0.0f, 0.0f, 0.0f, 1.0f},
        .border = 2.0f,
    };
    view->views[5] = (View2){
        .layer  = UILAYER_3,
        .kind   = VIEW2KIND_TEXT,
        .dims   = {dims.y * 0.5f},
        .offset = {dims.x * 0.5f, dims.y * 0.5f},
        .color  = {1.0f, 0.0f, 0.0f, 1.0f},
    };

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
        .kind   = PROJECTILEKIND_FIREBALL,
        .radius = size,
        .hitgen = Sol_Hitgen_Start(world),
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

    ScRibbon *ribbon         = Sol_Ribbon_AddKind(world, id, RIBBONKIND_FIRE);
    ribbon->rate             = 0.02f;
    ribbon->ribbon.thickness = size;
    Sol_Ribbon_Addpoint(&ribbon->ribbon, pos, 0, 0);

    return id;
}

int Sol_Prefab_PlasmaOrb(World *world, int owner, vec3s pos, vec3s dir, float speed, float size)
{
    int id = Sol_Create_Ent(world, pos);

    vec3s dims          = {size, size, size};
    ScBody3 *owner_body = Sol_Comp_Get(world, owner, ScBody3);

    *Sol_Comp_Add(world, id, ScOwner) = (ScOwner){
        .ownerId = owner,
    };

    *Sol_Comp_Add(world, id, ScBody3) = (ScBody3){
        .dims      = dims,
        .gravity   = GLMS_VEC3_ZERO,
        .is_sensor = true,
        .ignoreEnt = owner,
        .vel       = vecSca(dir, speed),
        .mask      = PHYSXMASK(COLLAYER_PROJECTILE, 0),
    };

    ScView3 *view3   = Sol_Comp_Add(world, id, ScView3);
    View3 plasma_orb = {
        .kind  = VIEW3KIND_PLASMAORB,
        .color = VEC4_RED,
        .scale = size,
    };
    solb_push(view3->views_b, plasma_orb);

    *Sol_Comp_Add(world, id, ScProjectile) = (ScProjectile){
        .kind   = PROJECTILEKIND_PLASMAORB,
        .radius = size,
        .hitgen = Sol_Hitgen_Start(world),
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

    ScTimer *timer  = Sol_Comp_Add(world, id, ScTimer);
    timer->destroy  = true;
    timer->duration = 2.0f;

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

    ScView2 *view2  = Sol_Comp_Add(world, id, ScView2);
    view2->count    = 1;
    view2->views[0] = (View2){.kind = VIEW2KIND_GREYITEMBANK};

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
    interact->flags      = INTERACTFLAG_DRAGGABLE;

    *Sol_Comp_Add(world, id, ScRef) = (ScRef){.kind = REFKIND_ABILITYBAR, .ent_world = ent_world_idx, .ent_id = ent_id};

    ScAbilitybar *ability_bar = Sol_Comp_Add(world, id, ScAbilitybar);
    ability_bar->slot_dims    = slot_dims;
    ability_bar->slot_count   = slot_count;
    memcpy(ability_bar->ability_slots, slots, sizeof(slots[0]) * slot_count);

    View2 bg        = {.dims = dims, .color = VEC4_BLACK, .hoverColor = VEC4_WHITE};
    View2 slot_view = {.kind = VIEW2KIND_ABILITYBAR, .dims = dims, .color = VEC4_BLACK, .layer = UILAYER_2};
    ScView2 *view2  = Sol_Comp_Add(world, id, ScView2);
    view2->count    = 2;
    view2->views[0] = bg;
    view2->views[1] = slot_view;

    // forc(Sol_GetWorldByIdx(ent_world_idx), ent_id, ScAbility)
    // {
    //     for (int i = 0; i < slot_count; i++)
    //     {
    //         if (c->base_actions[slots[i]] == 0)
    //             continue;
    //         vec3s card_pos = pos;
    //         card_pos.x += i * dims.x;
    //         Sol_User_AddItemLoose(&(SolItem){.item_kind = c->base_actions[slots[i]]});
    //     }
    // }

    return id;
}

int Sol_Prefab_AbilityCard(World *world, vec3s pos, SolItem *item)
{
    vec2s dims  = {62.0f, 62.0f};
    u32 texture = item_texture_map[item->item_kind];
    u32 layer   = UILAYER_1;

    int id = Sol_Create_Ent(world, pos);

    ScInteract *interact = Sol_Comp_Add(world, id, ScInteract);
    interact->flags      = INTERACTFLAG_DRAGGABLE;

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

    *Sol_Comp_Add(world, id, ScView2) = (ScView2){
        .count = 2,
        .views[0] =
            {
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
            },
        .views[1] =
            {
                .layer       = layer,
                .kind        = VIEW2KIND_RECT,
                .dims        = {dims.x, dims.y},
                .textureID   = SOL_TEXTURE_BORDER,
                .color       = rarity_color[item->rarity],
                .hoverColor  = {1.0f, 1.0f, 1.0f, 1.0f},
                .activeColor = {1, 1, 1, 1},
                .downColor   = {1, 1, 1, 1},
            },
    };

    return id;
}

int Sol_Prefab_DragonOrb(World *world, vec3s pos)
{
    int id = Sol_Create_Ent(world, pos);

    *Sol_Comp_Add(world, id, ScBody3) = sphere_body;

    ScView3 *view3   = Sol_Comp_Add(world, id, ScView3);
    View3 dragon_orb = {
        .kind  = VIEW3KIND_DRAGONORB,
        .color = {1, 1, 1, 1},
        .scale = 1.0f,
    };
    solb_push(view3->views_b, dragon_orb);

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

    ScRibbon *ribbon      = Sol_Ribbon_AddKind(world, id, RIBBONKIND_LIGHTNING);
    ribbon->rate          = 0.033f;
    ribbon->ribbon.sheets = 2;
    ribbon->ribbon.flags  = 1;
    Sol_Ribbon_Addpoint(&ribbon->ribbon, pos, 0.0f, 0);

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

    ScView2 *view2 = Sol_Comp_Add(world, id, ScView2);
    view2->count   = 2;
    View2 fill     = {
        .kind      = VIEW2KIND_POWERBAR,
        .color     = color,
        .dims      = dims,
        .textureID = Sol_TEXTURE_HEALTHVIALFILL,
        .fillSpeed = 5.0f,
        .flags     = 0b111,
    };
    View2 frame     = {.kind = VIEW2KIND_RECT, .color = color, .dims = dims, .textureID = Sol_TEXTURE_HEALTHVIAL};
    view2->views[0] = fill;
    view2->views[1] = frame;

    return id;
}