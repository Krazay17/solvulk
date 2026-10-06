#include "world.h"
#include "sol_user.h"
#include "sol_math.h"

void Sol_Test(World *world, double dt)
{
    return;
    int id = sol_user.view_ent;
    SolLine *line = Sol_Debug_NewLine(world, 0.2f);
    SolShoot shoot = Sol_Combat_GetShoot(world, id, 0.5f);
    line->a = shoot.pos;
    line->b = vecAdd(shoot.pos, vecSca(shoot.dir, 5.0f));
    line->aColor = line->bColor = VEC4_RED;
}