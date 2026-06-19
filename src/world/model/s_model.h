#pragma once
#include "base.h"
#include "model.h"

typedef struct World World;

typedef struct AnimDesc
{
    u8          playKind, force;
    float       blendIn, blendOut, seek, speed;
    AnimLayerId layerId;
    u32         anim;
} AnimDesc;

typedef struct
{
    SolModelKind id;
    float        yoffset, yawOffset;
} ModelDesc;

typedef struct AnimLayer
{
    u8    playKind;
    i16   currentAnim, lastAnim, animId;
    bool  force;
    float currentSeek, lastSeek;
    float blendFactor, blendSpeed;
    float fadeOut, fadeOutSpeed;
    float playRate;
} AnimLayer;
typedef struct CompModel
{
    mat4         bones[MAX_BONES];
    AnimLayer    layers[ANIM_LAYER_COUNT];
    SolModelKind modelId;
    u8           playingId[ANIM_LAYER_COUNT];
    bool         hasAnim, is2d;
    float        xOffset, yOffset, yawOffset;
} CompModel;

const extern CompModel model_kinds[SOL_MODEL_COUNT];

void       Sol_Model_Init(World *world);
CompModel *Sol_Model_Add(World *world, int id, SolModelKind kind, float height);
void       Sol_Model_Draw(World *world, double dt, double time);

void  Sol_Model_PlayAnim(World *world, int id, AnimDesc desc);
void  Sol_Model_SetAnimSpeed(World *world, int id, AnimLayerId layerId, float rate);
void  Sol_Model_SetAnimSeek(World *world, int id, AnimLayerId layerId, float seek);
vec3s Sol_Model_GetBoneXform(World *world, int id, const char *name);
float Sol_Model_GetOffsetY(World *world, int id);
void  Sol_Model_SetOffsetY(World *world, int id, float offset);
float Sol_Model_GetAnimSpeed(World *world, int id, AnimLayerId layerId);