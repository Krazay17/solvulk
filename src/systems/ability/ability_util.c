#include "sol_core.h"

#include "ability_i.h"

void Ability_BoostToDir(World *world, int id, vec3s dir, float speed)
{
    vec3s currentVel            = Sol_Physx_GetVel(world, id);
    float currentSpeedInJumpDir = glms_vec3_dot(currentVel, dir);

    if (currentSpeedInJumpDir < speed)
    {
        float speedToAdd = speed - currentSpeedInJumpDir;
        currentVel       = glms_vec3_add(currentVel, glms_vec3_scale(dir, speedToAdd));
    }

    Sol_Physx_SetVel(world, id, currentVel);
}
