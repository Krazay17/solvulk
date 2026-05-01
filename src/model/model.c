#include "model.h"
#include "render/render.h"
#include "sol_core.h"

CompModel *Sol_Model_Add(World *world, int id, CompModel init)
{
    CompModel model = init;
    model.model     = Sol_GetModel(model.modelId);
    if (!model.model->skeleton.animationCount)
        model.currentAnim = -1;

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

        if (modelComp->currentAnim < 0)
        {
            Sol_Submit_Model(modelComp->modelId, drawPos, xform->drawScale, xform->drawQuat, flags);
        }
        else
        {
            float duration         = modelComp->model->skeleton.animations[modelComp->currentAnim].duration;
            modelComp->currentSeek = fmodf(modelComp->currentSeek + fdt, duration);

            bool isBlending = (modelComp->lastAnim >= 0 && modelComp->blendFactor < 1.0f);

            if (isBlending)
            {
                float durationB     = modelComp->model->skeleton.animations[modelComp->lastAnim].duration;
                modelComp->lastSeek = fmodf(modelComp->lastSeek + fdt, durationB);

                modelComp->blendFactor += fdt * modelComp->blendSpeed;
                if (modelComp->blendFactor > 1.0f)
                    modelComp->blendFactor = 1.0f;
            }
            else
            {
                modelComp->blendFactor = 1.0f; // Force to 100% Anim A
            }
            AnimBlend blends = {
                .animA       = modelComp->currentAnim,
                .animB       = modelComp->lastAnim,
                .seekA       = modelComp->currentSeek,
                .seekB       = modelComp->lastSeek,
                .blendFactor = modelComp->blendFactor,
            };
            Sol_Skeleton_Pose(&modelComp->model->skeleton, &blends);

            SolDrawInstance drawInstance = {
                .pos     = drawPos,
                .rot     = xform->drawQuat,
                .scale   = xform->drawScale,
                .flags   = flags,
                .bonePtr = blends.bones,
            };
            Sol_Submit_Animated_Model(modelComp->modelId, &drawInstance);
        }
    }
}

void Sol_Model_PlayAnim(World *world, int id, SolAnims anim, float blendSpeed)
{
    CompModel *model = &world->models[id];
    if (!model->model->skeleton.animations)
        return;

    i32 nextAnim = model_anim_map[model->modelId][anim];
    if (nextAnim == model->currentAnim)
        return;

    model->lastAnim = model->currentAnim;
    model->lastSeek = model->currentSeek;

    model->currentAnim = nextAnim;
    model->currentSeek = 0;
    model->blendFactor = 0;
    model->blendSpeed  = blendSpeed > 0 ? blendSpeed : 5.0f;
}
