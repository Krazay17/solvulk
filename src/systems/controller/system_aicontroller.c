#include "sol_core.h"
#include <cglm/struct.h>

void Sol_System_Controller_Ai_Tick(World *world, double dt, double time)
{
    int required = HAS_CONTROLLER_AI;
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;

        CompController *controller = &world->controllers[id];
        CompXform *xform = &world->xforms[id];
        CompXform *playerXform = &world->xforms[world->playerID];
        if (!playerXform)
            return;

        vec3s lookdir = glms_vec3_normalize(glms_vec3_sub(playerXform->pos, xform->pos));
        float yaw = atan2f(lookdir.x, lookdir.z);
        float pitch = asinf(lookdir.y);

        controller->lookdir = lookdir;
        controller->wishdir = lookdir;
        controller->yaw = yaw;
        controller->pitch = pitch;
        
        xform->quat = Sol_Quat_FromYawPitch(yaw, 0);
    }
}