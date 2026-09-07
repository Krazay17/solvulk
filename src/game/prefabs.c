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

int Sol_Prefab_Dude(World *world, vec3s pos, float scale)
{
    int id         = Sol_Create_Ent(world, pos);
    ScModel *model = Sol_Comp_Add(world, id, ScModel);
    model->kind    = MODELKIND_DUDE;
    Sol_Anim_Add(world, id);
    ScBody3 *body     = Sol_Body3_Add(world, id);
    body->restitution = 0.01f;
    body->shape       = SHAPE3_CAP;
    body->dims        = (vec3s){0.5f, 1.7f, 0.5f};
    body->mask        = PHYSXMASK(COLLAYER_TEAMA, COLLAYER_ALL);

    ScCombat *combat  = Sol_Comp_Add(world, id, ScCombat);
    combat->healthMax = 100.0f;
    combat->health    = 100.0f;

    ScAbility *ability = Sol_Comp_Add(world, id, ScAbility);
    *ability           = (ScAbility){
        .action_map = {ABILITY_STATE_CLAW, ABILITY_STATE_FIREBALL, 0, 0, 0, 0, 0, 0, 0, ABILITY_STATE_DASH},
        .slots      = 10,
        .activeSlot = -1,
    };

    ScMove3 *move      = Sol_Comp_Add(world, id, ScMove3);
    move->kind         = MOVEMENTKIND_PLAYER;
    move->baseHeight   = body->dims.y;
    move->targetHeight = move->baseHeight;

    return id;
}

int Sol_Prefab_Wizard(World *world, vec3s pos, float scale)
{
    int id            = Sol_Create_Ent(world, pos);
    ScCombat *combat  = Sol_Comp_Add(world, id, ScCombat);
    combat->healthMax = 100.0f;
    combat->health    = 100.0f;
    ScMeta *meta      = Sol_Comp_Add(world, id, ScMeta);
    snprintf(meta->name, sizeof(meta->name), "Wizard %d", id);
    ScInteract *interact = Sol_Comp_Add(world, id, ScInteract);
    interact->range      = 5.0f;
    ScModel *model       = Sol_Comp_Add(world, id, ScModel);
    ScBody3 *body3       = Sol_Body3_Add(world, id);
    body3->shape         = SHAPE3_CAP;
    body3->mask          = PHYSXMASK(COLLAYER_TEAMZ, COLLAYER_ALL);
    body3->dims          = (vec3s){0.5f, 3.0f, 0.5f};
    model->kind          = MODELKIND_WIZARD;
    Sol_Anim_Add(world, id);

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

int Sol_Prefab_Button(World *world, vec3s pos, const char *text, u32 interact_mask)
{
    vec2s dims = {150.0f, 50.0f};

    int id = Sol_Create_Ent(world, pos);
    if (id < 0)
        return -1;
    ScInteract *interact = Sol_Comp_Add(world, id, ScInteract);
    interact->state |= interact_mask;

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
    int id        = Sol_Create_Ent(world, pos);
    ScBody3 *body = Sol_Comp_Add(world, id, ScBody3);

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
    body->mask      = PHYSXMASK(projLayer, targetFilter);
    body->ignoreEnt = owner;
    body->shape     = SHAPE3_SPH;
    body->mass = 1.0f;
    body->invMass = 1.0f;
    body->dims.x    = size;
    body->dims.y    = size;
    body->gravity = SOL_PHYS_GRAV;
    body->vel       = vecSca(dir, speed);
    ScView3 *view   = Sol_Comp_Add(world, id, ScView3);
    view->kind      = VIEW3KIND_FIREBALL;
    view->color     = VEC4_RED;
    view->dims.x    = size;

    return id;
}