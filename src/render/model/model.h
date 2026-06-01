#pragma once
#include "sol/types.h"

typedef struct
{
    float       blendIn, blendOut, seek, speed;
    SolAnimId   anim;
    AnimLayerId layerId;
    bool        force, oneShot;
} AnimDesc;

typedef struct
{
    SolModelId id;
    float      yoffset, yawOffset;
} ModelDesc;

typedef struct AnimLayer
{
    i16   currentAnim, lastAnim, animId;
    bool  oneShot;
    float currentSeek, lastSeek;
    float blendFactor, blendSpeed;
    float fadeOut, fadeOutSpeed;
    float playRate;
} AnimLayer;

typedef struct CompModel
{
    mat4       bones[MAX_BONES];
    AnimLayer  layers[ANIM_LAYER_COUNT];
    SolModelId modelId;
    u8         playingId[ANIM_LAYER_COUNT];
    bool       hasAnim;
    float      yOffset, yawOffset;
} CompModel;

u32 Sol_Model_GetTriCount(SolModelId handle);
void Transform_Tris_LocalToWorld(SolTri *group, int id, int offset, SolModelId handle, CompXform *xform);

void Sol_Model_Init(World *world);
void Sol_Model_Add(World *world, int id, ModelDesc desc);
void Sol_Model_Draw(World *world, double dt, double time);

SolModelId Sol_Model_GetModelId(World *world, int id);

void  Sol_Model_PlayAnim(World *world, int id, AnimDesc desc);
void  Sol_Model_SetAnimSpeed(World *world, int id, AnimLayerId layerId, float speedDif);
void  Sol_Model_SetAnimSeek(World *world, int id, AnimLayerId layerId, float seek);
vec3s Sol_Model_GetBoneXform(World *world, int id, const char *name);
float Sol_Model_GetOffsetY(World *world, int id);
void  Sol_Model_SetOffsetY(World *world, int id, float offset);

