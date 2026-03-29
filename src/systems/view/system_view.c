#include "sol_core.h"

void Sol_System_View_Draw(World *world, double dt, double time)
{
    int required = HAS_XFORM | HAS_MODEL;
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) == required)
        {
            CompXform *xform = &world->xforms[id];
            CompModel *model = &world->models[id];

            Sol_DrawModel(model->gpuHandle, (vec3){xform->pos.x, xform->pos.y, xform->pos.z}, 0);
        }
    }
}