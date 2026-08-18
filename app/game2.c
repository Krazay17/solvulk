#include "game.h"

static World *game;

void Create_Sol_Game()
{
    game = World_Create_Default(WORLDKIND_MENU);
    solEngine.activeWorld = game;
    int snake = Sol_Create_Ent(game, 0);
    Sol_Xform_Add(game, snake, (vec3s){500.0f, 500.0f, 0});
    Sol_Body2d_Add(game, snake, BODY2DKIND_RECT, 1, 1, PHYSXMASK(COLLISIONGROUP_PAWN, COLLISIONGROUP_WORLD));
    Sol_View2d_Add(game, snake, VIEW2DKIND_RECT, (vec4s){1.0f, 1.0f, 1.0f, 1.0f}, 10.0f, 10.0f);
}