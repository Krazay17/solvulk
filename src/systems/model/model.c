#include "model.h"
#include "sol_core.h"

CompModel *Sol_Model_Add(World *world, int id, CompModel init)
{
    CompModel model = init;
    model.model     = Sol_GetModel(model.modelId);
    if (!model.model->skeleton.animationCount)
        model.animIndex = -1;

    world->models[id] = model;
    world->masks[id] |= HAS_MODEL;
    return &world->models[id];
}

void Sol_System_Model_Draw(World *world, double dt, double time)
{
    float fdt      = (float)dt;
    int   required = HAS_XFORM | HAS_MODEL;
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;

        CompXform    *xform     = &world->xforms[id];
        CompModel    *modelComp = &world->models[id];
        CompInteract *interact  = (world->masks[id] & HAS_INTERACT) ? &world->interacts[id] : NULL;

        u32 flags = 0;
        if (interact)
        {
            if (interact->states & INTERACT_HOVERED || world->flags[id].flags & EFLAG_PICKEDUP)
                flags = 1;
        }

        vec3s drawPos = xform->drawPos;
        drawPos.y += modelComp->yOffset;

        if (modelComp->animIndex < 0)
        {
            Sol_Submit_Model(modelComp->modelId, drawPos, xform->drawScale, xform->drawQuat, flags);
        }
        else
        {
            Sol_Submit_Animated_Model(modelComp->modelId, drawPos, xform->drawScale, xform->drawQuat,
                                      modelComp->animIndex, modelComp->animSeek, flags);

            modelComp->animSeek += fdt;

            float duration = modelComp->model->skeleton.animations[modelComp->animIndex].duration;
            if (modelComp->animSeek >= duration)
            {
                modelComp->animSeek = 0;
            }
        }
    }
    Flush_Models();
    Flush_Models_Skinned();
}

void Sol_Model_PlayAnim(World *world, int id, SolAnims anim, float seek)
{
    CompModel *model = &world->models[id];
    if (!model->model->skeleton.animations)
        return;

    i32 nextAnim = model_anim_map[model->modelId][anim];
    if (nextAnim == model->animIndex)
        return;

    model->animIndex = nextAnim;
    model->animSeek  = seek;
}