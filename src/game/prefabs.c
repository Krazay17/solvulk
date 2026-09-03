#include "prefabs.h"
#include "world.h"

int Sol_Prefab_Dude(World *world, vec3s pos, float scale)
{
    int id = Sol_Create_Ent(world);
    Sol_Xform_Add(world, id, pos);
    ScModel *model = Sol_Comp_Add(world, id, ScModel);
    model->kind    = MODELKIND_DUDE;
    Sol_Anim_Add(world, id);
    ScBody3 *body      = Sol_Body3_Add(world, id);
    body->restitution  = 0.01f;
    body->shape        = SHAPE3_CAP;
    body->dims         = (vec3s){0.5f, 1.7f, 0.5f};
    body->mask         = PHYSXMASK(1, 1);
    ScMove3 *move      = Sol_Comp_Add(world, id, ScMove3);
    move->kind         = MOVEMENTKIND_PLAYER;
    move->baseHeight   = body->dims.y;
    move->targetHeight = move->baseHeight;

    return id;
}

int Sol_Prefab_Crosshair(World *world)
{
    int button = Sol_Create_Ent(world);
    Sol_Xform_Add(world, button, (vec3s){(float)WINDOW_WIDTH / 2.0f, (float)WINDOW_HEIGHT / 2.0f, 0});
    ScView2 *buttonView2  = Sol_Comp_Add(world, button, ScView2);
    buttonView2->count    = 1;
    buttonView2->views[0] = (View2){
        .kind       = VIEW2DKIND_RECT,
        .textureID  = SOL_TEXTURE_CROSSHAIR,
        .offset     = {-9.0f, -9.0f},
        .dims       = {18.0f, 18.0f},
        .color      = {1, 1, 1, 1},
        .scale      = 1.0f,
        .targetFill = 1.0f,
        .hoverColor = {1, 1, 1, 1},
    };
}