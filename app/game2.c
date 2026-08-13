#include "game.h"

static World *game;

void Create_Sol_Game()
{
    game = World_Create_Default(WORLDKIND_MENU);
    Sol_World_SetActive(game);
    int snake = Sol_Create_Ent(game, 0);
    Sol_Body2d_Add(game, snake, BODY2DKIND_RECT, 1, 1, PHYSXMASK(COLLISIONGROUP_PAWN, COLLISIONGROUP_WORLD));
}