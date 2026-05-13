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
    float       blendIn, blendOut, seek;
    SolAnims    anim;
    AnimLayerId layerId;
    bool        force;
} AnimDesc;

typedef struct
{
    SolModelId id;
    float      yoffset;
} ModelDesc;

void       Sol_Model_Init(World *world);
void       Sol_Model_Add(World *world, int id, ModelDesc desc);
void       Sol_Model_Draw(World *world, double dt, double time);
void       Sol_Model_PlayAnim(World *world, int id, AnimDesc desc);
void       Sol_Model_StopAnim(World *world, int id, AnimLayerId layerId, float fade);
SolModelId Sol_Model_GetModelId(World *world, int id);
