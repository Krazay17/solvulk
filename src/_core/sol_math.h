#pragma once
#include "sol/types.h"
#include <cglm/cglm.h>
#include <cglm/struct.h>

#define vecAdd(a, b) glms_vec3_add(a, b)
#define vecSub(a, b) glms_vec3_sub(a, b)
#define vecSca(a, b) glms_vec3_scale(a, b)
#define vecDot(a, b) glms_vec3_dot(a, b)
#define vecCrs(a, b) glms_vec3_cross(a, b)
#define vecLerp(a, b, c) glms_vec3_lerp(a, b, c)

#define SOL_COLOR(hex) (SolColor){.r = ((hex) >> 16) & 0xFF, .g = ((hex) >> 8) & 0xFF, .b = ((hex)) & 0xFF, .a = 255}

#define SOL_COLORA(hex, alpha)                                                                                         \
    (SolColor)                                                                                                         \
    {                                                                                                                  \
        .r = ((hex) >> 16) & 0xFF, .g = ((hex) >> 8) & 0xFF, .b = ((hex)) & 0xFF, .a = (alpha)                         \
    }

extern const vec3s VECTOR_RADIAL_DIRECTIONS[4];

SolVec3  Sol_Vec3_Add(SolVec3 a, SolVec3 b);
SolColor Sol_Color_Lerp(SolColor base, SolColor target, float alpha);

bool  Sol_Check_2d_Collision(vec2s a, vec4s b);
vec3s ClosestPointOnTriangle(vec3s p, vec3s a, vec3s b, vec3s c);

vec3s   Sol_Vec3_FromYawPitch(float yaw, float pitch);
versors Sol_Quat_FromYawPitch(float yaw, float pitch);
versors Sol_Quat_FromLookDir(vec3s lookDir);
versors Sol_Quat_FromLookDira(vec3s lookDir);
float   Sol_YawFromQuat(versor quat);

float Sol_Lerp(float start, float end, float amount);
float PulseAnim(float dt, float value, float speed);

static inline vec3s ClosestPointOnTriangle(const vec3s p, const vec3s a, const vec3s b, const vec3s c)
{
    const vec3s ab = glms_vec3_sub(b, a);
    const vec3s ac = glms_vec3_sub(c, a);
    const vec3s ap = glms_vec3_sub(p, a);

    const float d1 = glms_vec3_dot(ab, ap);
    const float d2 = glms_vec3_dot(ac, ap);
    if (d1 <= 0.0f && d2 <= 0.0f)
        return a;

    const vec3s bp = glms_vec3_sub(p, b);
    const float d3 = glms_vec3_dot(ab, bp);
    const float d4 = glms_vec3_dot(ac, bp);
    if (d3 >= 0.0f && d4 <= d3)
        return b;

    const vec3s cp = glms_vec3_sub(p, c);
    const float d5 = glms_vec3_dot(ab, cp);
    const float d6 = glms_vec3_dot(ac, cp);
    if (d6 >= 0.0f && d5 <= d6)
        return c;

    float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
    {
        const float v = d1 / (d1 - d3);
        return glms_vec3_add(a, glms_vec3_scale(ab, v));
    }

    float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
    {
        const float w = d2 / (d2 - d6);
        return glms_vec3_add(a, glms_vec3_scale(ac, w));
    }

    float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f)
    {
        const float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        return glms_vec3_add(b, glms_vec3_scale(glms_vec3_sub(c, b), w));
    }

    const float denom = va + vb + vc;
    if (denom < FLOATING_EPSILON)
        return a;

    const float inv = 1.0f / denom;
    const float v   = vb * inv;
    const float w   = vc * inv;
    return glms_vec3_add(a, glms_vec3_add(glms_vec3_scale(ab, v), glms_vec3_scale(ac, w)));
}