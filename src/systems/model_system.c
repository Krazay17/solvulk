#include "sol_core.h"

#include "xform/xform.h"
#include "render/render.h"

typedef enum
{
    ANIM_GROUP_MOVEMENT,
    ANIM_GROUP_ABILITY,
    ANIM_GROUP_COUNT,
} AnimGroupId;
typedef struct
{
    i32   currentAnim, lastAnim;
    float currentSeek, lastSeek;
    float blendFactor, blendSpeed;
} AnimGroup;
typedef struct CompModel
{
    SolModelId modelId;
    SolModel  *model;
    bool       hasAnim;
    float      yOffset;
    AnimGroup  anim[ANIM_GROUP_COUNT];
    mat4       bones[MAX_BONES];
} CompModel;

void Sol_Model_Init(World *world)
{
    world->models = calloc(MAX_ENTS, sizeof(CompModel));
}

void Sol_Model_Add(World *world, int id, ModelDesc desc)
{
    CompModel model = {.modelId = desc.id, .yOffset = desc.yoffset};
    model.model     = &Sol_Bank_Get()->models[model.modelId];
    if (!model.model->skeleton.animationCount)
        model.hasAnim = false;

    world->models[id] = model;
    world->masks[id] |= HAS_MODEL;
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

        CompXform    *xform     = &world->xforms[id];
        CompModel    *modelComp = &world->models[id];

        u32 flags = 0;
        if (world->masks[id] & HAS_INTERACT)
        {
            if (Sol_Interact_GetState(world, id) & INTERACT_HOVERED || world->flags[id].flags & EFLAG_PICKEDUP)
                flags = 1;
        }

        vec3s drawPos = xform->drawPos;
        drawPos.y += modelComp->yOffset;

        if (!modelComp->hasAnim)
        {
            Sol_Draw_Model(modelComp->modelId, drawPos, xform->drawScale, xform->drawQuat, flags);
        }
        else
        {
            // TODO UPDATE FOR ANIM GROUP

            // float duration         = modelComp->model->skeleton.animations[modelComp->currentAnim].duration;
            // modelComp->currentSeek = fmodf(modelComp->currentSeek + fdt, duration);

            // bool isBlending = (modelComp->lastAnim >= 0 && modelComp->blendFactor < 1.0f);

            // if (isBlending)
            // {
            //     float durationB     = modelComp->model->skeleton.animations[modelComp->lastAnim].duration;
            //     modelComp->lastSeek = fmodf(modelComp->lastSeek + fdt, durationB);

            //     modelComp->blendFactor += fdt * modelComp->blendSpeed;
            //     if (modelComp->blendFactor > 1.0f)
            //         modelComp->blendFactor = 1.0f;
            // }
            // else
            // {
            //     modelComp->blendFactor = 1.0f; // Force to 100% Anim A
            // }
            // AnimBlend blends = {
            //     .animA       = modelComp->currentAnim,
            //     .animB       = modelComp->lastAnim,
            //     .seekA       = modelComp->currentSeek,
            //     .seekB       = modelComp->lastSeek,
            //     .blendFactor = modelComp->blendFactor,
            // };
            // Sol_Skeleton_Pose(&modelComp->model->skeleton, &blends);

            // SolModelDraw drawInstance = {
            //     .pos     = drawPos,
            //     .rot     = xform->drawQuat,
            //     .scale   = xform->drawScale,
            //     .flags   = flags,
            //     .bonePtr = blends.bones,
            // };
            // Sol_Draw_Model_Skinned(modelComp->modelId, &drawInstance);
        }
    }
}

void Sol_Model_PlayAnim(World *world, int id, SolAnims anim, float blendSpeed)
{
    // TODO UPDATE FOR ANIM GROUP
    // CompModel *model = &world->models[id];
    // if (!model->model->skeleton.animations)
    //     return;

    // i32 nextAnim = model_anim_map[model->modelId][anim];
    // if (nextAnim == model->currentAnim)
    //     return;

    // model->lastAnim = model->currentAnim;
    // model->lastSeek = model->currentSeek;

    // model->currentAnim = nextAnim;
    // model->currentSeek = 0;
    // model->blendFactor = 0;
    // model->blendSpeed  = blendSpeed > 0 ? blendSpeed : 6.0f;
}

SolModel *Sol_Model_GetModel(World *world, int id)
{
    return world->models[id].model;
}