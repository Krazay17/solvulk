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
    int   required = HAS_XFORM | HAS_MODEL;

    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;

        CompXform *xform     = &world->xforms[id];
        CompModel *modelComp = &world->models[id];

        u32 flags = 0;
        if (world->masks[id] & HAS_INTERACT)
        {
            if (Sol_Interact_GetState(world, id) & INTERACT_HOVERED || world->flags[id].flags & EFLAG_PICKEDUP)
                flags |= (1 << 0);
        }
        if (Sol_Buff_HasBuff(world, id, BUFFKIND_INVULN))
            flags |= (1 << 1);

        vec3s drawPos = xform->drawPos;
        drawPos.y += modelComp->yOffset;

        ModelPushDesc desc = {
            .handle = modelComp->modelId,
        };

        desc.position.x = drawPos.x;
        desc.position.y = drawPos.y;
        desc.position.z = drawPos.z;
        desc.position.w = 1.0f;

        desc.rotation.x = xform->drawQuat.x;
        desc.rotation.y = xform->drawQuat.y;
        desc.rotation.z = xform->drawQuat.z;
        desc.rotation.w = xform->drawQuat.w;

        desc.scale.x = xform->drawScale.x;
        desc.scale.y = xform->drawScale.y;
        desc.scale.z = xform->drawScale.z;
        desc.scale.w = 1.0f;

        desc.flags = flags;

        if (!modelComp->hasAnim)
        {
            Sol_Render_PushModel(desc);
            continue;
        }
        desc.hasAnim = true;

        SolModel *m = Sol_GetModel(modelComp->modelId);

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

        // mat4        bones[MAX_BONES];
        PoseRequest req = {.outBones = modelComp->bones};

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
        desc.bones = modelComp->bones;
        Sol_Render_PushModel(desc);
    }
}

void Sol_Model_PlayAnim(World *world, int id, AnimDesc desc)
{
    CompModel *modelComp = &world->models[id];
    AnimLayer *layer     = &modelComp->layers[desc.layerId];
    i32        nextAnim  = model_anim_map[modelComp->modelId][desc.anim];
    if (!desc.force && nextAnim == layer->currentAnim)
        return;

    if (layer->blendFactor > 0.5f)
    {
        layer->lastAnim    = layer->currentAnim;
        layer->lastSeek    = layer->currentSeek;
        layer->blendFactor = 0;
    }
    layer->currentAnim = nextAnim;
    layer->currentSeek = desc.seek;
    layer->playRate    = desc.speed;
    layer->blendSpeed  = desc.blendIn > 0 ? desc.blendIn : 6.0f;

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