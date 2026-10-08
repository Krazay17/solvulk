#include "world.h"
#include "sol_user.h"
#include "sol_math.h"

void Sol_Test(World *world, double dt)
{
    return;
    int id         = sol_user.view_ent;
    SolLine *line  = Sol_Debug_NewLine(world, 0.2f);
    SolShoot shoot = Sol_Combat_GetShoot(world, id, 0.5f);
    line->a        = shoot.pos;
    line->b        = vecAdd(shoot.pos, vecSca(shoot.dir, 5.0f));
    line->aColor = line->bColor = VEC4_RED;

    Xform xform   = Xform_GetDraw(world, id);
    vec3s forward = glms_quat_rotatev(xform.rot, WORLD_FWD);

    printf("rot: %.3f %.3f %.3f %.3f  forward: %.3f %.3f %.3f\n", xform.rot.x, xform.rot.y, xform.rot.z, xform.rot.w,
           forward.x, forward.y, forward.z);
}