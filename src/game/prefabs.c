/*
 * File: prefabs.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-04
 *
 */
#include "prefabs.h"
#include "world.h"
#include "sol_math.h"

const ScCamera player_camera = {
    .fov              = 75.0f,
    .up.y             = 1.0f,
    .lerpspeed        = 10.0f,
    .desired_offset   = 1.0f,
    .desired_distance = 3.0f,
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
    .dims        = {0.5f, 1.0f, 0.5f},
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
    .respawnTime = 2.0f,
};

static const ScAbility dude_ability = {
    .base_actions =
        {
            ABILITY_STATE_FIREBALL,
            ABILITY_STATE_FIREBALL,
            ABILITY_STATE_CLAW,
            ABILITY_STATE_CLAW,
            ABILITY_STATE_CLAW,
            ABILITY_STATE_CLAW,
            ABILITY_STATE_DASH,
        },
    .slots = 7,
};

int Sol_Prefab_Dude(World *world, vec3s pos, float scale)
{
    int id = Sol_Create_Ent(world, pos);

    *Sol_Comp_Add(world, id, ScModel) = dude_model;
    *Sol_Comp_Add(world, id, ScAnim)  = anim_default;
    *Sol_Comp_Add(world, id, ScBody3) = dude_body;

    ScCombat *combat   = Sol_Comp_Add(world, id, ScCombat);
    *combat            = dude_combat;
    combat->respawnPos = pos;

    *Sol_Comp_Add(world, id, ScAbility) = dude_ability;
    *Sol_Comp_Add(world, id, ScMove3)   = dude_move;
    *Sol_Comp_Add(world, id, ScCamera)  = player_camera;

    *Sol_Comp_Add(world, id, ScView3) = (ScView3){
        .kind  = VIEW3KIND_HEALTHBAR,
        .color = {0.1f, 0.9f, 0.1f, 1.0f},
    };

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

    *Sol_Comp_Add(world, id, ScView3) = (ScView3){
        .kind  = VIEW3KIND_HEALTHBAR,
        .color = {0.1f, 0.9f, 0.1f, 1.0f},
    };

    *Sol_Comp_Add(world, id, ScCombat) = wizard_combat;
    *Sol_Comp_Add(world, id, ScBody3)  = wizard_body;
    *Sol_Comp_Add(world, id, ScAi)     = wizard_ai;

    *Sol_Comp_Add(world, id, ScAbility) = (ScAbility){.base_actions = {3, 3, 3, 3, 3, 3, 3}};

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
        .textureUV  = {0, 0, 1, 1},
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
    interact->state |= interact_flags;

    ScHook *hook  = Sol_Comp_Add(world, id, ScHook);
    hook->release = on_click;

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
    interact->state |= interact_flags;

    ScBody2 body = {
        .zindex = layer,
        .shape  = SHAPE2_REC,
        .dims   = {dims.x, dims.y, 0},
        .mask   = PHYSXMASK(COLLAYER_WORLD, COLLAYER_WORLD),
    };
    *Sol_Comp_Add(world, id, ScBody2) = body;

    ScSlider slider = {
        .axis      = {1, 0, 0},
        .track_len = dims.x,
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

int Sol_Prefab_Healthbar(World *world, vec3s pos)
{
    vec2s dims = {300.0f, 30.0f};

    int id               = Sol_Create_Ent(world, pos);
    ScInteract *interact = Sol_Comp_Add(world, id, ScInteract);
    interact->state |= INTERACT_DRAGGABLE;

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
        .kind       = VIEW2KIND_HEALTHBAR,
        .dims       = {dims.x, dims.y},
        .color      = {1.0f, 0.0f, 0.0f, 1.0f},
        .fillSpeed  = 1.5f,
        .hoverColor = {1, 1, 0, 0.5f},
        .textureID  = SOL_TEXTURE_HEALTH,
    };
    view->views[3] = (View2){
        .kind       = VIEW2KIND_HEALTHBAR,
        .dims       = {dims.x, dims.y},
        .color      = {0.0f, 1.0f, 0.0f, 1.0f},
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

    u32 projLayer    = COLLAYER_PROJECTILE; // Default fallback
    u32 targetFilter = COLLAYER_ALL;

    *Sol_Comp_Add(world, id, ScBody3) = (ScBody3){
        .dims      = dims,
        .gravity   = (vec3s)SOL_GRAVITY,
        .is_sensor = true,
        .ignoreEnt = owner,
        .vel       = vecSca(dir, speed),
        .mask      = PHYSXMASK(COLLAYER_PROJECTILE, 0),
    };

    *Sol_Comp_Add(world, id, ScView3) = (ScView3){
        .kind  = VIEW3KIND_FIREBALL,
        .color = VEC4_RED,
        .scale = size,
    };

    *Sol_Comp_Add(world, id, ScProjectile) = (ScProjectile){
        .kind   = PROJECTILEKIND_FIREBALL,
        .radius = size,
        .hitgen = Sol_Hitgen_Start(world, id),
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

    *Sol_Comp_Add(world, id, ScCombat) = (ScCombat){.health = 100.0f, .healthMax = 100.0f};

    return id;
}

int Sol_Prefab_Crystal(World *world, vec3s pos)
{
    return 0;
}

int Sol_Prefab_AbilityBar(World *world, vec3s pos, int slots)
{
    vec3s dims     = {434.0f, 62.0f};
    u32 layer      = UILAYER_0;
    float slotSize = dims.x / (float)slots;

    int id = Sol_Create_Ent(world, pos);

    Sol_Comp_Add(world, id, ScInteract)->state = INTERACT_DRAGGABLE;
    *Sol_Comp_Add(world, id, ScAbilitybar)     = (ScAbilitybar){
        .slots     = slots,
        .slot_dims = {slotSize, dims.y},
    };

    *Sol_Comp_Add(world, id, ScBody2) = (ScBody2){
        .zindex   = layer,
        .shape    = SHAPE2_REC,
        .dims.x   = dims.x,
        .dims.y   = dims.y,
        .isSensor = true,
        .mask     = PHYSXMASK(COLLAYER_ALL, COLLAYER_ALL),
    };

    *Sol_Comp_Add(world, id, ScView2) = (ScView2){.count = 5,
                                                  .views = {
                                                      {
                                                          .layer = layer,
                                                          .kind  = VIEW2KIND_RECT,
                                                          .dims  = {dims.x, dims.y},
                                                          .color = {0, 0, 0, 1},
                                                      },
                                                      {
                                                          .layer = layer,
                                                          .kind  = VIEW2KIND_ABILITYBAR_BASEICON,
                                                          .dims  = {dims.x, dims.y},
                                                          .color = {1.0f, 1.0f, 1.0f, 1.0f},
                                                          .desat = 1.0f,
                                                      },
                                                      {
                                                          .layer      = UILAYER_2,
                                                          .kind       = VIEW2KIND_ABILITYBAR,
                                                          .dims       = {dims.x, dims.y},
                                                          .flags      = 0b111,
                                                          .color      = {1, 0, 0, 1.0f},
                                                          .hoverColor = {0.7f, 0.7f, 0.7f, 1.0f},
                                                          .textureID  = SOL_TEXTURE_SWIRLFRAME,
                                                      },
                                                      {
                                                          .layer      = UILAYER_2,
                                                          .kind       = VIEW2KIND_RECT,
                                                          .dims       = {slotSize, slotSize},
                                                          .offset     = {-slotSize},
                                                          .textureID  = SOL_TEXTURE_TRIBOOKEND,
                                                          .color      = {1, 1, 1, 1},
                                                          .hoverColor = {0.7f, 0.7f, 0.7f, 1.0f},
                                                          .textureUV  = {0.01f, 0.0f, 1.0f, 1.0f},
                                                      },
                                                      {
                                                          .layer      = UILAYER_2,
                                                          .kind       = VIEW2KIND_RECT,
                                                          .dims       = {slotSize, slotSize},
                                                          .offset     = {dims.x},
                                                          .textureUV  = {0.0f, 0, -1.0f, 0},
                                                          .textureID  = SOL_TEXTURE_TRIBOOKEND,
                                                          .color      = {1, 1, 1, 1},
                                                          .hoverColor = {0.7f, 0.7f, 0.7f, 1.0f},
                                                      },
                                                  }};

    return id;
}

int Sol_Prefab_AbilityCard(World *world, vec3s pos, AbilityState ability, int ref)
{
    vec2s dims  = {62.0f, 62.0f};
    u32 texture = ability_texture_map[ability];
    u32 layer   = UILAYER_1;

    int id = Sol_Create_Ent(world, pos);

    Sol_Comp_Add(world, id, ScInteract)->state = INTERACT_ONLYDRAGGABLE;

    *Sol_Comp_Add(world, id, ScRef) = (ScRef){.kind = REFKIND_ITEM, .index = ref};

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
                .textureUV   = {0, 0, 1.0f, 0.816f},
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
                .color       = {0.0f, 0.0f, 0.0f, 1.0f},
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

    *Sol_Comp_Add(world, id, ScView3) = (ScView3){
        .kind  = VIEW3KIND_DRAGONORB,
        .color = {1, 1, 1, 1},
        .scale = 1.0f,
    };
}

int Sol_Prefab_PlasmaOrb(World *world, vec3s pos)
{
    int id = Sol_Create_Ent(world, pos);

    *Sol_Comp_Add(world, id, ScBody3) = sphere_body;

    *Sol_Comp_Add(world, id, ScView3) = (ScView3){
        .kind  = VIEW3KIND_PLASMAORB,
        .color = {1, 1, 1, 1},
        .scale = 1.0f,
    };
}