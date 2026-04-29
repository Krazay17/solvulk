#include <cglm/cglm.h>

#include "sol_core.h"

CompModel *Sol_Model_Add(World *world, int id, CompModel init)
{
    CompModel model = init;
    model.model = Sol_GetModel(model.modelId);

    world->models[id] = model;
    world->masks[id] |= HAS_MODEL;
    return &world->models[id];
}

void Sol_System_Model_Draw(World *world, double dt, double time)
{
    int required = HAS_XFORM | HAS_MODEL;
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;

        CompXform *xform     = &world->xforms[id];
        CompModel *modelComp = &world->models[id];
        vec3s      drawPos   = xform->drawPos;
        drawPos.y += modelComp->yOffset;

        Sol_Submit_Model(modelComp->modelId, drawPos, xform->drawScale, xform->drawQuat);
    }
}
