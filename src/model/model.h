#pragma once
#include "sol/types.h"

typedef enum
{
    ANIM_LAYER_BASE,  // full body, always active
    ANIM_LAYER_LOWER, // overrides legs
    ANIM_LAYER_UPPER, // overrides torso/arms
    ANIM_LAYER_OVERRIDE,
    ANIM_LAYER_COUNT
} AnimLayerId;

typedef struct
{
    float       blendIn, blendOut, seek, speed;
    SolAnimId   anim;
    AnimLayerId layerId;
    bool        force;
} AnimDesc;

typedef struct
{
    SolModelId id;
    float      yoffset, yawOffset;
} ModelDesc;

void Sol_Model_Init(World *world);
void Sol_Model_Add(World *world, int id, ModelDesc desc);
void Sol_Model_Draw(World *world, double dt, double time);

SolModelId Sol_Model_GetModelId(World *world, int id);

void  Sol_Model_PlayAnim(World *world, int id, AnimDesc desc);
void  Sol_Model_StopAnim(World *world, int id, AnimLayerId layerId, float fade);
void  Sol_Model_SetAnimSpeed(World *world, int id, AnimLayerId layerId, float speedDif);
vec3s Sol_Model_GetBoneXform(World *world, int id, const char *name);
float Sol_Model_GetOffsetY(World *world, int id);
void Sol_Model_SetOffsetY(World *world, int id, float offset);