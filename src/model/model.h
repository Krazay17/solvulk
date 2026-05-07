#pragma once
#include "sol/types.h"

typedef struct BoneMask
{
    bool layerOwns[MAX_BONES]; // for one layer
} BoneMask;

typedef struct AnimLayer
{
    i32   currentAnim, lastAnim;
    float currentSeek, lastSeek;
    float blendFactor, blendSpeed;
    float fadeOut, fadeOutSpeed;
} AnimLayer;

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
    AnimLayer  layers[ANIM_LAYER_COUNT];
} CompModel;

// Per model, per layer
typedef struct SolModelMasks
{
    BoneMask layers[ANIM_LAYER_COUNT];
} SolModelMasks;

typedef struct AnimBlend
{
    int   anim;     // current animation index, or -1 to skip layer
    int   lastAnim; // previous, or -1 for no fade
    float seek, lastSeek;
    float blendFactor; // current cross-fade within this layer (0..1)
} AnimBlend;

typedef struct PoseRequest
{
    AnimBlend layers[ANIM_LAYER_COUNT];      // per-layer state
    BoneMask  masks[ANIM_LAYER_COUNT];       // which bones each layer owns
    float     layerWeight[ANIM_LAYER_COUNT]; // how strongly each layer applies (0..1)
    mat4     *outBones;                      // final skinning matrices
} PoseRequest;

extern SolModelMasks model_masks[SOL_MODEL_COUNT];
const char          *model_path[SOL_MODEL_COUNT];
const i32            model_anim_map[SOL_MODEL_COUNT][ANIM_COUNT];

SolModel *Sol_GetModel(SolModelId id);

void      Sol_Load_Models();
SolModel *Parse_Model(SolResource res, u32 id);
void      Init_Anim_Masks(SolModelId modelId, SolSkeleton *skele);
void      Mark_Bone_And_Descendants(SolSkeleton *skel, int boneIdx, BoneMask *mask);
int       Sol_Skeleton_FindBone(SolSkeleton *skel, const char *name);

void Sol_Skeleton_Pose(SolSkeleton *skel, PoseRequest *req);
void Sol_Draw_Model(SolModelId handle, vec3s pos, vec3s scale, versors quat, u32 flags);
void Sol_Draw_Model_Skinned(SolModelId handle, SolModelDraw *inst);
