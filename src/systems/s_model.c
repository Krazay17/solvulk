#include "sol_core.h"

#include "model.h"

void Sol_Model_Init(World *world)
{
    world->models = calloc(MAX_ENTS, sizeof(CompModel));
}

void Sol_Model_Add(World *world, int id, ModelDesc desc)
{
    world->masks[id] |= HAS_MODEL;

    CompModel model = {.modelId = desc.id, .yOffset = desc.yoffset};
    SolModel *m     = Sol_GetModel(desc.id);
    if (m->skeleton.animationCount > 0)
    {
        model.hasAnim = true;
        for (int i = 0; i < ANIM_LAYER_COUNT; i++)
        {
            model.layers[i].currentAnim = -1;
            model.layers[i].currentSeek = 0;
        }
    }

    world->models[id] = model;
}

void Sol_Model_Draw(World *world, double dt, double time)
{
    float fdt      = (float)dt;
    int   required = HAS_XFORM | HAS_MODEL;

    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;

        CompXform   *xform     = &world->xforms[id];
        CompModel   *modelComp = &world->models[id];

        u32 flags = 0;
        if (world->masks[id] & HAS_INTERACT)
        {
            if (Sol_Interact_GetState(world, id) & INTERACT_HOVERED || world->flags[id].flags & EFLAG_PICKEDUP)
                flags = 1;
        }

        vec3s drawPos = xform->drawPos;
        drawPos.y += modelComp->yOffset;

        ModelSSBO modelSSBO = {0};
        FlagsSSBO flagsSSBO = {0};

        modelSSBO.position[0] = drawPos.x;
        modelSSBO.position[1] = drawPos.y;
        modelSSBO.position[2] = drawPos.z;
        modelSSBO.position[3] = 1.0f;

        modelSSBO.rotation[0] = xform->drawQuat.x;
        modelSSBO.rotation[1] = xform->drawQuat.y;
        modelSSBO.rotation[2] = xform->drawQuat.z;
        modelSSBO.rotation[3] = xform->drawQuat.w;

        modelSSBO.scale[0] = xform->drawScale.x;
        modelSSBO.scale[1] = xform->drawScale.y;
        modelSSBO.scale[2] = xform->drawScale.z;
        modelSSBO.scale[3] = 1.0f;

        flagsSSBO.flags = flags;

        if (!modelComp->hasAnim)
        {
            Sol_Render_Push_Model(modelComp->modelId, &modelSSBO, &flagsSSBO);
            continue;
        }

        SolModel *m = Sol_GetModel(modelComp->modelId);

        for (int L = 0; L < ANIM_LAYER_COUNT; L++)
        {
            AnimLayer *layer = &modelComp->layers[L];
            if (layer->currentAnim < 0)
                continue;

            float dur          = m->skeleton.animations[layer->currentAnim].duration;
            layer->currentSeek = fmodf(layer->currentSeek + fdt, dur);

            if (layer->lastAnim >= 0 && layer->blendFactor < 1.0f)
            {
                float lastDur   = m->skeleton.animations[layer->lastAnim].duration;
                layer->lastSeek = fmodf(layer->lastSeek + fdt, lastDur);
                layer->blendFactor += fdt * layer->blendSpeed;
                if (layer->blendFactor >= 1.0f)
                {
                    layer->blendFactor = 1.0f;
                    layer->lastAnim    = -1;
                }
            }

            // Fade-out countdown
            if (layer->fadeOut > 0.0f)
            {
                layer->fadeOut -= fdt * layer->fadeOutSpeed;
                if (layer->fadeOut < 0.0f)
                    layer->fadeOut = 0.0f;
                if (layer->fadeOut == 0.0f)
                {
                    layer->currentAnim = -1;
                    layer->lastAnim    = -1;
                }
            }
        }

        // Build pose request
        BonesSSBO   bonesSSBO = {0};
        PoseRequest req       = {.outBones = bonesSSBO.bones};

        for (int L = 0; L < ANIM_LAYER_COUNT; L++)
        {
            req.layers[L] = (AnimBlend){
                .anim        = modelComp->layers[L].currentAnim,
                .lastAnim    = modelComp->layers[L].lastAnim,
                .seek        = modelComp->layers[L].currentSeek,
                .lastSeek    = modelComp->layers[L].lastSeek,
                .blendFactor = modelComp->layers[L].blendFactor,
            };
            req.masks[L]       = model_masks[modelComp->modelId].layers[L];
            req.layerWeight[L] = (modelComp->layers[L].fadeOut > 0.0f) ? modelComp->layers[L].fadeOut : 1.0f;
        }
        Sol_Skeleton_Pose(&m->skeleton, &req);
        Sol_Render_Push_Model_Skinned(modelComp->modelId, &modelSSBO, &flagsSSBO, &bonesSSBO);
    }
}

static DoPlayAnim(World *world, int id, SolAnims anim, float blendSpeed, AnimLayerId layerId, float seek)
{
    CompModel *modelComp = &world->models[id];
    AnimLayer *layer     = &modelComp->layers[layerId];
    i32        nextAnim  = model_anim_map[modelComp->modelId][anim];
    layer->lastAnim      = layer->currentAnim;
    layer->lastSeek      = layer->currentSeek;
    layer->currentAnim   = nextAnim;
    layer->currentSeek   = seek;
    layer->blendFactor   = 0;
    layer->blendSpeed    = blendSpeed > 0 ? blendSpeed : 6.0f;
    layer->fadeOut       = 0.0f;
    layer->fadeOutSpeed  = 0.0f;
}

void Sol_Model_PlayAnim(World *world, int id, AnimDesc desc)
{
    CompModel *modelComp = &world->models[id];
    AnimLayer *layer     = &modelComp->layers[desc.layerId];
    i32        nextAnim  = model_anim_map[modelComp->modelId][desc.anim];
    if (!desc.force && nextAnim == layer->currentAnim)
        return;
    layer->lastAnim     = layer->currentAnim;
    layer->lastSeek     = layer->currentSeek;
    layer->currentAnim  = nextAnim;
    layer->currentSeek  = desc.seek;
    layer->blendFactor  = 0;
    layer->blendSpeed   = desc.blendIn > 0 ? desc.blendIn : 6.0f;
    layer->fadeOut      = 0.0f;
    layer->fadeOutSpeed = desc.force ? 0 : desc.blendOut;
}

SolModelId Sol_Model_GetModelId(World *world, int id)
{
    return world->models[id].modelId;
}

void Sol_Model_StopAnim(World *world, int id, AnimLayerId layerId, float fade)
{
    AnimLayer *layer = &world->models[id].layers[layerId];
    if (layer->currentAnim < 0)
        return;

    if (fade <= 0.0f)
    {
        // Instant stop
        layer->currentAnim = -1;
        layer->lastAnim    = -1;
        layer->fadeOut     = 0.0f;
    }
    else
    {
        layer->fadeOut      = 1.0f;
        layer->fadeOutSpeed = 1.0f / fade;
    }
}