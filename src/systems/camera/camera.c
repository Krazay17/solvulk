#include "sol_core.h"
#include <cglm/struct.h>
#include "camera.h"

#include "render.h"

void Sol_System_Camera_Tick(World *world, double dt, double time)
{
    int required = HAS_CONTROLLER | HAS_XFORM | HAS_BODY3;
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) == required)
        {
            CompXform *xform = &world->xforms[id];
            CompController *controller = &world->controllers[id];

            vec3s pos = glms_vec3_sub(xform->pos, glms_vec3_scale(controller->wishdir, 10.0f));
            vec3 finalPos = {pos.x, pos.y, pos.z};
            vec3s targetS = glms_vec3_add(pos, controller->wishdir);
            vec3 finalTarget = {targetS.x, targetS.y, targetS.z};

            Sol_Camera_Update(finalPos, finalTarget);

            break;
        }
    }
}

void Sol_Fov_Set(float fov)
{
    renderCam.fov = fov;
}