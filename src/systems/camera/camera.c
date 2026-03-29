#include <cglm/struct.h>

#include "sol_core.h"
#include "render.h"

void Sol_System_Camera_Tick(World *world, double dt, double time)
{
    int required = HAS_CONTROLLER | HAS_XFORM;
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) == required)
        {
            CompXform *xform = &world->xforms[id];
            CompController *controller = &world->controllers[id];

            vec3s pos = glms_vec3_sub(xform->pos, glms_vec3_scale(controller->lookdir, 10.0f));
            vec3s target = xform->pos;

            vec3 finalPos = {pos.x, pos.y, pos.z};
            vec3 finalTarget = {target.x, target.y, target.z};

            Sol_Camera_Update(finalPos, finalTarget);
            Sol_Debug_Add("CamPosX", pos.x);
            Sol_Debug_Add("CamPosY", pos.y);
            Sol_Debug_Add("CamPosZ", pos.z);

            break;
        }
    }
}

void Sol_Fov_Set(float fov)
{
    renderCam.fov = fov;
}