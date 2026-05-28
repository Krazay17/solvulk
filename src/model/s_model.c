#include "sol_core.h"

#include "model_i.h"

void Sol_Model_Init(World *world)
{
    world->draw3dSystems[world->draw3dCount++] = Sol_Model_Draw;

    world->models = calloc(MAX_ENTS, sizeof(CompModel));
}

void Sol_Model_Add(World *world, int id, ModelDesc desc)
{
    world->masks[id] |= HAS_MODEL;

    CompModel model = {.modelId = desc.id, .yOffset = desc.yoffset, .yawOffset = desc.yawOffset};
    SolModel *m     = Sol_GetModel(desc.id);
    if (m->skeleton.animationCount > 0)
    {
        model.hasAnim = true;
        for (int i = 0; i < ANIM_LAYER_COUNT; i++)
        {
            model.layers[i].currentAnim = -1;
            model.layers[i].currentSeek = 0;
            model.layers[i].blendFactor = 0;
        }
    }
    world->models[id] = model;
}

void Sol_Model_Draw(World *world, double dt, double time)
{
    float fdt      = (float)dt;
    int   required = HAS_ACTIVE | HAS_XFORM | HAS_MODEL;

    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;
        if (Sol_Vital_GetDead(world, id))
            continue;

        CompXform *xform     = &world->xforms[id];
        CompModel *modelComp = &world->models[id];

        ModelSSBO modelSSBO = {0};
        if (world->masks[id] & HAS_INTERACT)
            if (Sol_Interact_GetState(world, id) & INTERACT_HOVERED || world->flags[id].flags & EFLAG_PICKEDUP)
                modelSSBO.flags |= (1 << 0);

        if (Sol_Buff_HasBuff(world, id, BUFFKIND_INVULN))
            modelSSBO.flags |= (1 << 1);

        if (world->masks[id] & HAS_VITAL)
            modelSSBO.hitTime = Sol_Vital_GetLastHitTime(world, id);
        else
            modelSSBO.hitTime = -100.0f;

        vec3s drawPos = xform->drawPos;
        drawPos.y += modelComp->yOffset;

        modelSSBO.position[0] = drawPos.x;
        modelSSBO.position[1] = drawPos.y;
        modelSSBO.position[2] = drawPos.z;
        modelSSBO.position[3] = 1.0f;
        modelSSBO.rotation[0] = xform->drawQuat.x;
        modelSSBO.rotation[1] = xform->drawQuat.y;
        modelSSBO.rotation[2] = xform->drawQuat.z;
        modelSSBO.rotation[3] = xform->drawQuat.w;
        modelSSBO.scale[0]    = xform->drawScale.x;
        modelSSBO.scale[1]    = xform->drawScale.y;
        modelSSBO.scale[2]    = xform->drawScale.z;
        modelSSBO.scale[3]    = 1.0f;

        if (!modelComp->hasAnim)
        {
            Sol_Render_GetNext_Model(modelComp->modelId, &modelSSBO, NULL);
            continue;
        }
        BonesSSBO bonesSSBO = {0};
        SolModel *m         = Sol_GetModel(modelComp->modelId);

        for (int L = 0; L < ANIM_LAYER_COUNT; L++)
        {
            AnimLayer *layer = &modelComp->layers[L];
            if (layer->currentAnim < 0)
                continue;

            float dur          = m->skeleton.animations[layer->currentAnim].duration;
            float speed        = layer->playRate != 0 ? layer->playRate * fdt : fdt;
            float newSeek      = layer->currentSeek + speed;
            layer->currentSeek = fmodf(fmodf(newSeek, dur) + dur, dur);

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

        PoseRequest req = {.outBones = bonesSSBO.bones};

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
        memcpy(modelComp->bones, bonesSSBO.bones, sizeof(modelComp->bones));
        Sol_Render_GetNext_Model(modelComp->modelId, &modelSSBO, &bonesSSBO);
    }
}

void Sol_Model_PlayAnim(World *world, int id, AnimDesc desc)
{
    CompModel *modelComp = &world->models[id];
    AnimLayer *layer     = &modelComp->layers[desc.layerId];
    float      blendOut  = desc.blendOut > 0 ? desc.blendOut : 0.25f;
    float      speed     = desc.speed > 0 ? desc.speed : 1.0f;

    if (!desc.force && desc.anim == modelComp->playing[desc.layerId])
        return;

    modelComp->playing[desc.layerId] = desc.anim;

    if (desc.anim == 0)
    {

        if (blendOut <= 0.0f)
        {
            // Instant stop
            layer->currentAnim = -1;
            layer->lastAnim    = -1;
            layer->fadeOut     = 0.0f;
        }
        else
        {
            layer->fadeOut      = 1.0f;
            layer->fadeOutSpeed = 1.0f / blendOut;
        }
        return;
    }

    i32 mappedAnim = model_anim_map[modelComp->modelId][desc.anim];
    if (layer->blendFactor > 0.5f)
    {
        layer->lastAnim    = layer->currentAnim;
        layer->lastSeek    = layer->currentSeek;
        layer->blendFactor = 0;
    }
    layer->currentAnim = mappedAnim;
    layer->currentSeek = desc.seek;
    layer->playRate    = speed;
    layer->blendSpeed  = desc.blendIn > 0 ? desc.blendIn : 6.0f;

    layer->fadeOut      = 0.0f;
    layer->fadeOutSpeed = desc.force ? 0 : blendOut;
}

SolModelId Sol_Model_GetModelId(World *world, int id)
{
    return world->models[id].modelId;
}

void Sol_Model_SetAnimSpeed(World *world, int id, AnimLayerId layerId, float speedDif)
{
    AnimLayer *layer = &world->models[id].layers[layerId];
    if (layer->currentAnim < 0)
        return;
    layer->playRate = speedDif;
}

void Sol_Model_SetTint(World *world, int id, vec4s color)
{
}

vec3s Sol_Model_GetBoneXform(World *world, int id, const char *name)
{
    CompModel   *model    = &world->models[id];
    SolSkeleton *skeleton = &loaded_models[model->modelId].skeleton;
    CompXform   *xform    = &world->xforms[id];

    int boneIdx = -1;
    for (int i = 0; i < skeleton->boneCount; i++)
    {
        if (strstr(skeleton->bones[i].name, name))
        {
            boneIdx = i;
            break;
        }
    }
    if (boneIdx < 0)
        return (vec3s){{0, 0, 0}};

    // Get the bone's bind position in model space (from inverseBind)
    // inverseBind transforms model-space to bone-local. Its inverse goes the other way.
    // The bone's origin in model space at bind pose is: inverse(inverseBind) * (0,0,0,1)
    // Equivalently: -inverseBind[3].xyz with rotation undone, OR just invert and grab translation.
    mat4 bindPose;
    glm_mat4_inv(skeleton->bones[boneIdx].inverseBind, bindPose);
    vec3 bindPos = {bindPose[3][0], bindPose[3][1], bindPose[3][2]};

    // Transform bind position by the current skinning matrix to get current model-space pos
    vec4 bindPos4 = {bindPos[0], bindPos[1], bindPos[2], 1.0f};
    vec4 modelSpacePos;
    glm_mat4_mulv(model->bones[boneIdx], bindPos4, modelSpacePos);

    // Now apply the entity's world transform (position + rotation + scale)
    vec3 scaledPos = {
        modelSpacePos[0] * xform->drawScale.x,
        modelSpacePos[1] * xform->drawScale.y,
        modelSpacePos[2] * xform->drawScale.z,
    };

    // Rotate by entity quat
    vec3 rotated;
    glm_quat_rotatev(xform->drawQuat.raw, scaledPos, rotated);

    // Translate
    vec3s worldPos = {{
        rotated[0] + xform->drawPos.x,
        rotated[1] + xform->drawPos.y + model->yOffset,
        rotated[2] + xform->drawPos.z,
    }};

    return worldPos;
}

float Sol_Model_GetOffsetY(World *world, int id)
{
    return world->models[id].yawOffset;
}
void Sol_Model_SetOffsetY(World *world, int id, float offset)
{
    CompModel *model = &world->models[id];
    model->yOffset   = offset;
}