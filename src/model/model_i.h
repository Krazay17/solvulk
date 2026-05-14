#pragma once
#include "sol/types.h"

#include "model.h"

typedef struct SolBone
{
    char name[64];
    int  parent;      // index into bones array, or -1 for root
    mat4 inverseBind; // baked from glTF

    // Local TRS at rest (used as default if no animation channel exists)
    vec3s   restTrans;
    versors restRot;
    vec3s   restScale;
} SolBone;

// One animation channel = one bone's TRS curve
typedef enum
{
    ANIM_PATH_TRANSLATION,
    ANIM_PATH_ROTATION,
    ANIM_PATH_SCALE,
} AnimPath;

typedef struct SolAnimChannel
{
    int      boneIndex; // which bone this affects
    AnimPath path;      // T, R, or S
    float   *times;     // keyframe timestamps, length = keyCount
    float   *values;    // packed values: vec3 for T/S, vec4 (quat) for R
    int      keyCount;
} SolAnimChannel;

typedef struct SolAnimation
{
    char            name[64];
    float           duration; // longest keyframe time across all channels
    SolAnimChannel *channels;
    int             channelCount;
} SolAnimation;

typedef struct SolSkeleton
{
    SolBone      *bones;
    int           boneCount;
    SolAnimation *animations;
    int           animationCount;
} SolSkeleton;

typedef struct SolMesh
{
    uint32_t    vertexOffset;
    uint32_t    vertexCount;
    uint32_t    indexOffset;
    uint32_t    indexCount;
    SolMaterial material;
} SolMesh;

typedef struct SolModel
{
    SolVertex *vertices;
    SolMesh   *meshes;
    SolTri    *tris;
    u32       *indices;

    u32 vertex_count;
    u32 mesh_count;
    u32 tri_count;
    u32 indice_count;

    SolSkeleton skeleton;

    mat4s *jointMatrices;

    SolModelId modelId;
} SolModel;

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
    float playRate;
} AnimLayer;

typedef struct
{
    i32   currentAnim, lastAnim;
    float currentSeek, lastSeek;
    float blendFactor, blendSpeed;
} AnimGroup;

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


typedef struct CompModel
{
    SolModelId modelId;
    bool       hasAnim;
    float      yOffset;
    AnimLayer  layers[ANIM_LAYER_COUNT];
} CompModel;

extern SolModelMasks model_masks[SOL_MODEL_COUNT];
const char          *model_path[SOL_MODEL_COUNT];
const i32            model_anim_map[SOL_MODEL_COUNT][ANIM_COUNT];

SolModel  *Sol_GetModel(SolModelId id);

int  Sol_Models_Init();
void Init_Anim_Masks(SolModelId modelId, SolSkeleton *skele);
void Mark_Bone_And_Descendants(SolSkeleton *skel, int boneIdx, BoneMask *mask);
int  Sol_Skeleton_FindBone(SolSkeleton *skel, const char *name);
void Sol_Skeleton_Pose(SolSkeleton *skel, PoseRequest *req);
