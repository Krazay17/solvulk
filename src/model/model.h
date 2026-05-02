#pragma once
#include "sol/types.h"

typedef struct SolSkeleton SolSkeleton;
typedef struct SolBank     SolBank;

typedef struct
{
    int   animA, animB;
    float seekA, seekB;
    float blendFactor;
    mat4  bones[MAX_BONES];
} AnimBlend;

void     Load_Models(SolBank *bank);
SolModel Parse_Model(SolResource res);

void Sol_Skeleton_Pose(SolSkeleton *skel, AnimBlend *blends);

const char *model_path[SOL_MODEL_COUNT];
const i32   model_anim_map[SOL_MODEL_COUNT][ANIM_COUNT];

// Apply a 4x4 world matrix to a position
static inline void TransformPos(const float m[16], const float in[3], float out[3])
{
    out[0] = m[0] * in[0] + m[4] * in[1] + m[8] * in[2] + m[12];
    out[1] = m[1] * in[0] + m[5] * in[1] + m[9] * in[2] + m[13];
    out[2] = m[2] * in[0] + m[6] * in[1] + m[10] * in[2] + m[14];
}

// Apply only the rotation/scale part of a 4x4 matrix to a normal
// (no translation; for non-uniform scale you'd need the inverse-transpose,
// but for uniform scale + rotation this is fine)
static inline void TransformNrm(const float m[16], const float in[3], float out[3])
{
    out[0] = m[0] * in[0] + m[4] * in[1] + m[8] * in[2];
    out[1] = m[1] * in[0] + m[5] * in[1] + m[9] * in[2];
    out[2] = m[2] * in[0] + m[6] * in[1] + m[10] * in[2];

    // Renormalize (handles uniform scale)
    float len = sqrtf(out[0] * out[0] + out[1] * out[1] + out[2] * out[2]);
    if (len > 1e-8f)
    {
        out[0] /= len;
        out[1] /= len;
        out[2] /= len;
    }
}
