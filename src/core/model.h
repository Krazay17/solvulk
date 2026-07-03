#pragma once
#include "types.h"
#include "xform/s_xform.h"

#define MAX_BONES 128

typedef enum
{
    MODELKIND_WIZARD,
    MODELKIND_DUDE,
    MODELKIND_ZORGON,
    MODELKIND_WEAPONBLADE,
    SOL_MODEL_BOX,
    SOL_MODEL_WORLD0,
    SOL_MODEL_WORLD1,
    SOL_MODEL_WORLD2,
    SOL_MODEL_WORLD6,
    SOL_MODEL_COUNT,
} SolModelKind;

typedef enum
{
    ANIM_LAYER_BASE,  // full body, always active
    ANIM_LAYER_LOWER, // overrides legs
    ANIM_LAYER_UPPER, // overrides torso/arms
    ANIM_LAYER_OVERRIDE,
    ANIM_LAYER_COUNT
} AnimLayerId;

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

typedef struct SolMaterial
{
    float baseColor[4];
    float emissive[4];
    float metallic;
    float roughness;
    
    u32   textureId;
    u32   emissiveTextureId;
    u32   normalTextureId;
    u32   fogTextureId;

    vec2s textureScale;
    vec2s fogTextureScale;

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

    SolModelKind modelId;
} SolModel;

typedef struct BoneMask
{
    bool layerOwns[MAX_BONES]; // for one layer
} BoneMask;

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

extern SolModel      loaded_models[SOL_MODEL_COUNT];
extern SolModelMasks model_masks[SOL_MODEL_COUNT];
const char          *model_path[SOL_MODEL_COUNT];
const i32            model_anim_map[SOL_MODEL_COUNT][ANIM_COUNT];

int          Sol_Models_Init();
void         Init_Anim_Masks(SolModelKind modelId, SolSkeleton *skele);
void         Mark_Bone_And_Descendants(SolSkeleton *skel, int boneIdx, BoneMask *mask);
int          Sol_Skeleton_FindBone(SolSkeleton *skel, const char *name);
void         Sol_Skeleton_Pose(SolSkeleton *skel, PoseRequest *req);
u32          Sol_Model_GetTriCount(SolModelKind handle);
void         Transform_Tris_LocalToWorld(SolTri *group, int id, int offset, SolModelKind handle, CompXform *xform);
SolModelKind Sol_Model_GetModelId(World *world, int id);

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