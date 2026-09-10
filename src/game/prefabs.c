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

static const ScCamera player_camera = {
    .fov              = 80.0f,
    .up.y             = 1.0f,
    .lerpspeed        = 10.0f,
    .desired_offset   = 1.0f,
    .desired_distance = 2.0f,
};

static const ScBody3 wizard_body = {
    .shape       = SHAPE3_CAP,
    .mass        = 1.0f,
    .invMass     = 1.0f,
    .restitution = 0.2f,
    .gravity     = SOL_GRAVITY,
    .dims        = {0.5f, 3.0f, 0.5f},
    .mask        = PHYSXMASK(COLLAYER_TEAMZ, COLLAYER_ALL),
};

static const ScCombat wizard_combat = {
    .healthMax = 100.0f,
    .health    = 100.0f,
};

static const ScAi wizard_ai = {
    .kind       = AIKIND_WIZARD,
    .aggroRange = 20.0f,
};

static const ScBody3 dude_body = {
    .shape       = SHAPE3_CAP,
    .mass        = 1.0f,
    .invMass     = 1.0f,
    .restitution = 0.01f,
    .gravity     = SOL_GRAVITY,
    .dims        = {0.5f, 1.8f, 0.5f},
    .mask        = PHYSXMASK(COLLAYER_TEAMA, COLLAYER_ALL),
};

static const ScMove3 dude_move = {
    .kind       = MOVEMENTKIND_DUDE,
    .baseHeight = 1.8f,
};

static const ScCombat dude_combat = {
    .healthMax = 100.0f,
    .health    = 100.0f,
};

static const ScAbility dude_ability = {
    .action_map = {ABILITY_STATE_CLAW, ABILITY_STATE_FIREBALL, 0, 0, 0, 0, 0, 0, 0, ABILITY_STATE_DASH},
    .slots      = 10,
    .activeSlot = -1,
};

int Sol_Prefab_Dude(World *world, vec3s pos, float scale)
{
    int id = Sol_Create_Ent(world, pos);

    Sol_Anim_Add(world, id, MODELKIND_DUDE);

    *Sol_Comp_Add(world, id, ScBody3)   = dude_body;
    *Sol_Comp_Add(world, id, ScCombat)  = dude_combat;
    *Sol_Comp_Add(world, id, ScAbility) = dude_ability;
    *Sol_Comp_Add(world, id, ScCamera)  = player_camera;
    *Sol_Comp_Add(world, id, ScMove3)   = dude_move;

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

    Sol_Anim_Add(world, id, MODELKIND_WIZARD);

    *Sol_Comp_Add(world, id, ScCombat) = wizard_combat;
    *Sol_Comp_Add(world, id, ScBody3)  = wizard_body;
    *Sol_Comp_Add(world, id, ScAi)     = wizard_ai;

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
    };

    return id;
}

int Sol_Prefab_Button(World *world, vec3s pos, const char *text, u32 interact_flags)
{
    vec2s dims = {150.0f, 50.0f};

    int id = Sol_Create_Ent(world, pos);
    if (id < 0)
        return -1;
    ScInteract *interact = Sol_Comp_Add(world, id, ScInteract);
    interact->state |= interact_flags;

    ScBody2 *body = Sol_Comp_Add(world, id, ScBody2);
    *body         = (ScBody2){
        .shape = SHAPE2_REC,
        .dims  = {dims.x, dims.y, 0},
        .mask  = PHYSXMASK(COLLAYER_WORLD, COLLAYER_WORLD),
    };

    ScView2 *view  = Sol_Comp_Add(world, id, ScView2);
    view->count    = 4;
    view->views[0] = (View2){
        .kind        = VIEW2KIND_RECT,
        .dims        = {dims.x, dims.y},
        .color       = {0.1f, 0.1f, 0.1f, 1.0f},
        .hoverColor  = {1.0f, 1.0f, 1.0f, 1.0f},
        .toggleColor = {0.0f, 0.5f, 0.5f, 1.0f},
    };
    view->views[1] = (View2){
        .kind        = VIEW2KIND_RECT,
        .dims        = {dims.x, dims.y},
        .color       = {0.5f, 0.1f, 0.1f, 1.0f},
        .hoverColor  = {1.0f, 1.0f, 1.0f, 1.0f},
        .toggleColor = {0.0f, 0.5f, 0.5f, 1.0f},
        .textureID   = SOL_TEXTURE_SWIRLFRAME,
    };
    view->views[2] = (View2){
        .kind   = VIEW2KIND_RECT,
        .dims   = {dims.x, dims.y},
        .color  = {0.0f, 0.0f, 0.0f, 1.0f},
        .border = 3.0f,
    };
    view->views[3] = (View2){
        .kind   = VIEW2KIND_TEXT,
        .dims   = {16.0f},
        .color  = {0.0f, 1.0f, 0.0f, 1.0f},
        .offset = {dims.x * 0.5f, dims.y * 0.5f},
    };
    strncpy(view->views[3].text, text, sizeof(view->views[3].text));

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
    };

    ScView2 *view  = Sol_Comp_Add(world, id, ScView2);
    view->count    = 5;
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
        .kind       = VIEW2KIND_RECT,
        .dims       = {dims.x, dims.y},
        .color      = {1.0f, 0.0f, 0.0f, 1.0f},
        .fillSpeed  = 4.0f,
        .hoverColor = {1, 1, 0, 0.5f},
        .textureID  = SOL_TEXTURE_HEALTH,
    };
    view->views[3] = (View2){
        .kind       = VIEW2KIND_RECT,
        .dims       = {dims.x, dims.y},
        .color      = {0.0f, 1.0f, 0.0f, 1.0f},
        .fillSpeed  = 4.0f,
        .hoverColor = {1, 1, 0, 0.5f},
        .textureID  = SOL_TEXTURE_HEALTH,
    };
    view->views[4] = (View2){
        .kind   = VIEW2KIND_RECT,
        .dims   = {dims.x, dims.y},
        .color  = {0.0f, 0.0f, 0.0f, 1.0f},
        .border = 2.0f,
    };

    return id;
}

int Sol_Prefab_Fireball(World *world, int owner, vec3s pos, vec3s dir, float speed, float size)
{
    int id            = Sol_Create_Ent(world, pos);
    ScBody3 *body     = Sol_Comp_Add(world, id, ScBody3);
    body->dims        = (vec3s){size, size, size};
    body->gravity     = (vec3s)SOL_GRAVITY;
    body->mass        = 1.0f;
    body->invMass     = 1.0f;
    body->restitution = 1.0f;
    body->ignoreEnt   = owner;
    body->vel         = vecSca(dir, speed);

    ScBody3 *owner_body = Sol_Comp_Get(world, owner, ScBody3);
    u32 projLayer       = COLLAYER_TEAMA_PROJ; // Default fallback
    u32 targetFilter    = COLLAYER_ALL;
    if (owner_body)
    {
        u32 ownerLayer = PHYSX_GET_LAYER(owner_body->mask);

        // Projectile layer is 1 bit higher than team layer (TEAMA -> TEAMA_PROJ)
        projLayer = ownerLayer << 1;

        // Target everything EXCEPT friendly body layer and friendly projectile layer
        targetFilter = COLLAYER_WORLD | (COLLAYER_ALL & ~(ownerLayer | projLayer));
    }

    body->mask    = PHYSXMASK(projLayer, targetFilter);
    ScView3 *view = Sol_Comp_Add(world, id, ScView3);
    view->kind    = VIEW3KIND_FIREBALL;
    view->color   = VEC4_RED;
    view->dims.x  = size;

    return id;
}

int Sol_Prefab_Crystal(World *world, vec3s pos)
{
}