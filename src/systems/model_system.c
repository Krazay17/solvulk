#include "sol_core.h"

#include "render/render.h"
#include "xform/xform.h"

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
    bool       hasAnim;
    float      yOffset;
    AnimGroup  anim[ANIM_GROUP_COUNT];
} CompModel;

void Sol_Model_Init(World *world)
{
    world->models = calloc(MAX_ENTS, sizeof(CompModel));
}

void Sol_Model_Add(World *world, int id, ModelDesc desc)
{
    world->masks[id] |= HAS_MODEL;

    CompModel model = {.modelId = desc.id, .yOffset = desc.yoffset};
    SolModel *m     = &Sol_Bank_Get()->models[desc.id];
    if (m->skeleton.animationCount > 0)
        model.hasAnim = true;

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
            int j = 0;
            SolModel *m = &Sol_Bank_Get()->models[modelComp->modelId];
            float duration         = m->skeleton.animations[modelComp->anim[j].currentAnim].duration;
            modelComp->anim[j].currentSeek = fmodf(modelComp->anim[j].currentSeek + fdt, duration);

            bool isBlending = (modelComp->anim[j].lastAnim >= 0 && modelComp->anim[j].blendFactor < 1.0f);

            if (isBlending)
            {
                float durationB     = m->skeleton.animations[modelComp->anim[j].lastAnim].duration;
                modelComp->anim[j].lastSeek = fmodf(modelComp->anim[j].lastSeek + fdt, durationB);

                modelComp->anim[j].blendFactor += fdt * modelComp->anim[j].blendSpeed;
                if (modelComp->anim[j].blendFactor > 1.0f)
                    modelComp->anim[j].blendFactor = 1.0f;
            }
            else
            {
                modelComp->anim[j].blendFactor = 1.0f; // Force to 100% Anim A
            }
            AnimBlend blends = {
                .animA       = modelComp->anim[j].currentAnim,
                .animB       = modelComp->anim[j].lastAnim,
                .seekA       = modelComp->anim[j].currentSeek,
                .seekB       = modelComp->anim[j].lastSeek,
                .blendFactor = modelComp->anim[j].blendFactor,
            };
            Sol_Skeleton_Pose(&m->skeleton, &blends);

            SolModelDraw drawInstance = {
                .pos     = drawPos,
                .rot     = xform->drawQuat,
                .scale   = xform->drawScale,
                .flags   = flags,
                .bonePtr = blends.bones,
            };
            Sol_Draw_Model_Skinned(modelComp->modelId, &drawInstance);
        }
    }
}

void Sol_Model_PlayAnim(World *world, int id, SolAnims anim, float blendSpeed)
{
    // TODO UPDATE FOR ANIM GROUP
    int j = 0;
    CompModel *modelComp = &world->models[id];

    i32 nextAnim = model_anim_map[modelComp->modelId][anim];
    if (nextAnim == modelComp->anim[j].currentAnim)
        return;

    modelComp->anim[j].lastAnim = modelComp->anim[j].currentAnim;
    modelComp->anim[j].lastSeek = modelComp->anim[j].currentSeek;

    modelComp->anim[j].currentAnim = nextAnim;
    modelComp->anim[j].currentSeek = 0;
    modelComp->anim[j].blendFactor = 0;
    modelComp->anim[j].blendSpeed  = blendSpeed > 0 ? blendSpeed : 6.0f;
}

SolModelId Sol_Model_GetModelId(World *world, int id)
{
    return world->models[id].modelId;
}