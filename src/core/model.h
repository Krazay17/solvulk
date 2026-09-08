#pragma once
#include "sol/types.h"

#define BLEND_SPEED_DEFAULT 0.2f

typedef enum
{
    ANIMPLAYKIND_LOOP,
    ANIMPLAYKIND_NOLOOP,
    ANIMPLAYKIND_ONESHOT,
} AnimPlayKind;

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

typedef struct ScAnimChannel
{
    int      boneIndex; // which bone this affects
    AnimPath path;      // T, R, or S
    float   *times;     // keyframe timestamps, length = keyCount
    float   *values;    // packed values: vec3 for T/S, vec4 (quat) for R
    int      keyCount;
} ScAnimChannel;

typedef struct ScAnimation
{
    char            name[64];
    float           duration; // longest keyframe time across all channels
    ScAnimChannel *channels;
    int             channelCount;
} ScAnimation;

typedef struct SolSkeleton
{
    SolBone      *bones;
    int           boneCount;
    ScAnimation *animations;
    int           animationCount;
} SolSkeleton;

typedef struct SolMaterial
{
    float baseColor[4];
    float emissive[4];

    vec2s textureScale;
    vec2s fogTextureScale;

    float metallic;
    float roughness;

    u32 textureId;
    u32 emissiveTextureId;
    u32 normalTextureId;
    u32 fogTextureId;

    u32 _pad[2];
} SolMaterial;

typedef struct SolMesh
{
    uint32_t    vertexOffset;
    uint32_t    vertexCount;
    uint32_t    indexOffset;
    uint32_t    indexCount;
    SolMaterial material;
} SolMesh;

typedef struct ModelPrefab
{
    char  name[32];
    vec3s pos;
} ModelPrefab;

typedef struct ScModelData
{
    ModelKind    kind;
    SolVertex   *vertices;
    u32         *indices;
    SolMesh     *meshes;
    SolTri      *tris;
    ModelPrefab *prefabs;

    u32 vertex_count;
    u32 mesh_count;
    u32 tri_count;
    u32 indice_count;
    u32 prefab_count;

    SolSkeleton skeleton;

    mat4s *jointMatrices;

} ScModelData;

typedef struct BoneMask
{
    bool layerOwns[MAX_BONES]; // for one layer
} BoneMask;

// Per model, per layer
typedef struct ScModelDataMasks
{
    BoneMask layers[ANIM_LAYER_COUNT];
} ScModelDataMasks;

// typedef struct
// {
//     int   currentAnim, lastAnim;
//     float currentSeek, lastSeek;
//     float blendFactor, blendInSpeed;
// } AnimGroup;

// typedef struct AnimBlend
// {
//     int   anim;     // current animation index, or -1 to skip layer
//     int   lastAnim; // previous, or -1 for no fade
//     float seek, lastSeek;
//     float blendFactor; // current cross-fade within this layer (0..1)
//     bool  hasSnapshot;
// } AnimBlend;

// typedef struct PoseRequest
// {
//     AnimBlend layers[ANIM_LAYER_COUNT];      // per-layer state
//     BoneMask  masks[ANIM_LAYER_COUNT];       // which bones each layer owns
//     float     layerWeight[ANIM_LAYER_COUNT]; // how strongly each layer applies (0..1)
//     mat4     *outBones;                      // final skinning matrices
// } PoseRequest;

extern ScModelData      loaded_models[MODELKIND_COUNT];
extern ScModelDataMasks model_masks[MODELKIND_COUNT];
extern const char       *model_path[MODELKIND_COUNT];
extern const i32         model_anim_map[MODELKIND_COUNT][ANIM_COUNT];

int  Sol_Models_Init();
void Init_Anim_Masks(ModelKind kind, SolSkeleton *skele);
void Mark_Bone_And_Descendants(SolSkeleton *skel, int boneIdx, BoneMask *mask);
int  Sol_Skeleton_FindBone(SolSkeleton *skel, const char *name);
void Sol_Skeleton_Pose(int model_handle, SolPose *outPose, AnimLayer *layers, SolPoseE *lastPose, bool *hasLastPose);
// void           Sol_Skeleton_Pose(SolSkeleton *skel, PoseRequest *req);
// void Sol_Skeleton_Pose(int model_handle, SolPose *pose, AnimLayer *layers);
u32  Sol_Model_GetTriCount(ModelKind handle);
void Transform_Tris_LocalToWorld(SolTri *group, int id, int offset, ModelKind handle, versors quat, vec3s scale,
                                 vec3s pos);

static inline float Sol_GetExtrasFloat(const char *json_string, const char *key, float default_value)
{
    if (!json_string)
        return default_value;

    const char *found = strstr(json_string, key);
    if (!found)
        return default_value;

    found = strchr(found, ':');
    if (!found)
        return default_value;
    found++; // Step over ':'

    // Skip any potential spaces or opening quotes if Blender formats with whitespace
    while (*found == ' ' || *found == '"')
    {
        found++;
    }

    // atof stops at the first non-numeric character (like a comma ',' or bracket '}'),
    // but validating it protects your data floats from breaking
    return (float)atof(found);
}
