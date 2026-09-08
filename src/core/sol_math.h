/*
 * File: sol.h
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * Math!
 */
#pragma once
#include "sol/types.h"

#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#include "cglm/struct.h"

#define vecAdd(a, b) glms_vec3_add(a, b)
#define vecSub(a, b) glms_vec3_sub(a, b)
#define vecSca(a, b) glms_vec3_scale(a, b)
#define vecDot(a, b) glms_vec3_dot(a, b)
#define vecNorm(a) glms_vec3_normalize(a)
#define vecCrs(a, b) glms_vec3_cross(a, b)
#define vecLerp(a, b, c) glms_vec3_lerp(a, b, c)
#define vecDist(a, b) glms_vec3_distance(a, b)

#define SOL_COLOR(hex) (vec4s){.r = ((hex) >> 16) & 0xFF, .g = ((hex) >> 8) & 0xFF, .b = ((hex)) & 0xFF, .a = 255}

#define SOL_COLORA(hex, alpha)                                                                                         \
    (vec4s)                                                                                                            \
    {                                                                                                                  \
        .r = ((hex) >> 16) & 0xFF, .g = ((hex) >> 8) & 0xFF, .b = ((hex)) & 0xFF, .a = (alpha)                         \
    }

// CONFIG-------------------------------

extern const vec3s VECTOR_RADIAL_DIRECTIONS[9];

// FUNCS------------------------------------
vec3s Sol_Vec3_FromYawPitch(float yaw, float pitch);
versors Sol_Quat_FromYawPitch(float yaw, float pitch);
versors Sol_Quat_FromLookDir(vec3s lookDir);
versors Sol_Quat_FromLookDira(vec3s lookDir);

// INLINES-------------------
static inline StrafeDir Sol_GetStrafedirYaw(float x, float z, float yaw)
{
    // 1. Get the relative angle between velocity and facing direction
    float angle = atan2f(x, z) - yaw;

    // 2. Keep the angle strictly positive within [0, 2PI]
    if (angle < 0.0f)
        angle += 2.0f * GLM_PIf;

    // 3. Offset by half a wedge (22.5 degrees) so cardinal directions
    // sit right in the middle of our integer sectors instead of on the edges.
    angle += GLM_PI_4f * 0.5f;

    // 4. Handle wrapping after the offset addition
    if (angle >= 2.0f * GLM_PIf)
        angle -= 2.0f * GLM_PIf;

    // 5. Use floorf to establish clean boundaries, then cast
    int sector = (int)floorf(angle / GLM_PI_4f);

    // Safety clamp to guarantee it maps to your 0-7 enum range
    return (StrafeDir)(sector & 7);
}

static inline StrafeDir Sol_GetStrafedir(float x, float z, float bX, float bZ)
{
    // 1. Get the relative angle between velocity and facing direction
    float angle = atan2f(x, z) - atan2f(bX, bZ);

    // 2. Keep the angle strictly positive within [0, 2PI]
    if (angle < 0.0f)
        angle += 2.0f * GLM_PIf;

    // 3. Offset by half a wedge (22.5 degrees) so cardinal directions
    // sit right in the middle of our integer sectors instead of on the edges.
    angle += GLM_PI_4f * 0.5f;

    // 4. Handle wrapping after the offset addition
    if (angle >= 2.0f * GLM_PIf)
        angle -= 2.0f * GLM_PIf;

    // 5. Use floorf to establish clean boundaries, then cast
    int sector = (int)floorf(angle / GLM_PI_4f);

    // Safety clamp to guarantee it maps to your 0-7 enum range
    return (StrafeDir)(sector & 7);
}

static inline bool Sol_Check_2d_Collision(vec2s a, vec4s b)
{
    return !((a.x < b.x) | (a.x >= b.x + b.z) | (a.y < b.y) | (a.y >= b.y + b.w));
}

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

static inline float Sol_Math_Lerp(float start, float end, float amount)
{
    return start + amount * (end - start);
}

static inline float Sol_Math_Lerp_Clamped(float start, float end, float amount)
{
    amount = fmaxf(0, fminf(1.0f, amount));
    return start + amount * (end - start);
}

static inline float Sol_Math_MapRange(float startA, float endA, float startB, float endB, float amount)
{
    if (amount == 0)
        return 0.0f;
    return Sol_Math_Lerp(startA, endA, amount / (endB - startB));
}

static inline float Sol_Quat_ToYaw(versors q)
{
    // Extract yaw (rotation around Y axis) from unit quaternion
    // atan2(2*(w*y + x*z), 1 - 2*(y^2 + z^2))
    // Simplified for pure Y-axis rotations:
    return atan2f(2.0f * (q.w * q.y + q.x * q.z), 1.0f - 2.0f * (q.y * q.y + q.z * q.z));
}

static inline float Sol_YawFromQuat(versor q)
{
    return atan2f(2.0f * (q[1] * q[2] + q[3] * q[0]), q[3] * q[3] - q[0] * q[0] - q[1] * q[1] + q[2] * q[2]);
}

static inline float Sol_YawFromVec(vec3s v)
{
    return atan2f(v.x, v.z);
}

static inline float Sol_PitchFromVec(vec3s v)
{
    return asinf(v.y);
}

static inline float Sol_Math_RandRange(float a, float b)
{
    return Sol_Math_Lerp(a, b, (float)rand() / (float)RAND_MAX);
}

static inline vec3s Sol_RotFromQuat(versors quat)
{
    mat4s mat = glms_quat_mat4(quat);
    return glms_mat4_mulv3(mat, WORLD_FORWARD, 1.0f);
}

static inline float Sol_Math_RandRange2(float min, float max)
{
    return min + (float)rand() / (float)RAND_MAX * (max - min);
}

static inline vec3s Sol_Math_DampDir(vec3s vel, vec3s dir, float alpha, float damping, float dt)
{
    float speedInJumpDir = glms_vec3_dot(vel, dir);

    if (speedInJumpDir > 0.0f)
    {
        // Bleed off velocity along the jump vector over time
        float speedLoss = speedInJumpDir * damping * alpha * dt;
        // Apply the reduction along the jump track
        vel = glms_vec3_sub(vel, glms_vec3_scale(dir, speedLoss));
    }
    return vel;
}

/// @brief Redirects Velocity
/// @param vel incoming vel vector
/// @param dir Target direction (must be normalized)
static inline vec3s Sol_RedirectVel(vec3s vel, vec3s dir)
{
    float mag = glms_vec3_norm(vel);
    return vecSca(dir, mag);
}

static inline int get_index_from_mask(unsigned int mask)
{
    if (mask == 0)
        return -1; // Error or no bit set

    int index = 0;
    while ((mask & 1) == 0)
    {
        mask >>= 1;
        index++;
    }
    return index;
}

static inline vec3s Sol_ProjectVec(vec3s a, vec3s b)
{
    float dot = glms_vec3_dot(a, b);
    return glms_vec3_sub(a, glms_vec3_scale(b, dot));
}

static inline vec3s Sol_BounceVec(vec3s a, vec3s b)
{
    float dot = glms_vec3_dot(a, b);
    dot *= 2;
    return glms_vec3_sub(a, glms_vec3_scale(b, dot));
}

static inline vec3s CalcWishdir3(uint32_t action, vec3s lookdir, vec3s updir, bool includeY)
{
    vec3s wishdir  = {0, 0, 0};
    vec3s flatdir  = lookdir;
    flatdir.y      = 0;
    flatdir        = glms_vec3_normalize(flatdir);
    vec3s rightdir = glms_vec3_normalize(glms_vec3_cross(flatdir, updir));
    if (action & BITC(ACTION_FWD))
        wishdir = glms_vec3_add(wishdir, flatdir);

    if (action & BITC(ACTION_BWD))
        wishdir = glms_vec3_sub(wishdir, flatdir);

    if (action & BITC(ACTION_RIGHT))
        wishdir = glms_vec3_add(wishdir, rightdir);

    if (action & BITC(ACTION_LEFT))
        wishdir = glms_vec3_sub(wishdir, rightdir);

    if (includeY)
    {
        if (action & BITC(ACTION_JUMP))
            wishdir = glms_vec3_add(wishdir, updir);

        if (action & BITC(ACTION_CROUCH))
            wishdir = glms_vec3_sub(wishdir, updir);
    }

    float len2 = glms_vec3_norm2(wishdir);
    if (len2 <= 0.00001f)
        return (vec3s){0, 0, 0};

    return glms_vec3_scale(wishdir, 1.0f / sqrtf(len2));
}

static inline vec3s CalcWishDir2(uint32_t action)
{
    vec3s wishdir = {0};
    if (action & BITC(ACTION_RIGHT))
        wishdir.x += 1;
    if (action & BITC(ACTION_LEFT))
        wishdir.x -= 1;
    if (action & BITC(ACTION_FWD))
        wishdir.y -= 1;
    if (action & BITC(ACTION_BWD))
        wishdir.y += 1;
    if (action & BITC(ACTION_JUMP))
        wishdir.z += 1;
    if (action & BITC(ACTION_CROUCH))
        wishdir.z -= 1;

    float len2 = glms_vec3_norm2(wishdir);
    if (len2 <= 0.00001f)
        return (vec3s){0, 0, 0};

    return glms_vec3_scale(wishdir, 1.0f / sqrtf(len2));
}

static inline void Closest_Points_Segment_Segment(vec3s p1, vec3s q1, // segment A: p1 → q1
                                                  vec3s p2, vec3s q2, // segment B: p2 → q2
                                                  vec3s *outA, vec3s *outB)
{
    vec3s d1 = glms_vec3_sub(q1, p1); // segment A direction
    vec3s d2 = glms_vec3_sub(q2, p2); // segment B direction
    vec3s r  = glms_vec3_sub(p1, p2);

    float a = glms_vec3_dot(d1, d1); // squared length of segment A
    float e = glms_vec3_dot(d2, d2); // squared length of segment B
    float f = glms_vec3_dot(d2, r);

    float s, t;
    // Both segments degenerate to points?
    if (a <= FLOATING_EPSILON && e <= FLOATING_EPSILON)
    {
        *outA = p1;
        *outB = p2;
        return;
    }

    if (a <= FLOATING_EPSILON)
    {
        // Segment A is a point
        s = 0.0f;
        t = f / e;
        t = fmaxf(0.0f, fminf(1.0f, t));
    }
    else
    {
        float c = glms_vec3_dot(d1, r);

        if (e <= FLOATING_EPSILON)
        {
            // Segment B is a point
            t = 0.0f;
            s = fmaxf(0.0f, fminf(1.0f, -c / a));
        }
        else
        {
            // General case
            float b     = glms_vec3_dot(d1, d2);
            float denom = a * e - b * b;

            // Segments not parallel
            if (denom != 0.0f)
            {
                s = (b * f - c * e) / denom;
                s = fmaxf(0.0f, fminf(1.0f, s));
            }
            else
            {
                // Parallel — pick s = 0 arbitrarily
                s = 0.0f;
            }

            t = (b * s + f) / e;

            // Clamp t and recompute s if needed
            if (t < 0.0f)
            {
                t = 0.0f;
                s = fmaxf(0.0f, fminf(1.0f, -c / a));
            }
            else if (t > 1.0f)
            {
                t = 1.0f;
                s = fmaxf(0.0f, fminf(1.0f, (b - c) / a));
            }
        }
    }

    *outA = glms_vec3_add(p1, glms_vec3_scale(d1, s));
    *outB = glms_vec3_add(p2, glms_vec3_scale(d2, t));
}

static inline vec3s Closest_Point_Segment_Point(vec3s s0, vec3s s1, vec3s p)
{
    vec3s seg      = glms_vec3_sub(s1, s0);
    float segLenSq = glms_vec3_dot(seg, seg);

    if (segLenSq < 0.0001f)
        return s0;

    vec3s pDelta = glms_vec3_sub(p, s0);
    float t      = glms_vec3_dot(pDelta, seg) / segLenSq;
    t            = glm_clamp(t, 0.0f, 1.0f);

    return glms_vec3_add(s0, glms_vec3_scale(seg, t));
}

static inline void compose_trs(vec3 pos, versor quat, vec3 scale, mat4 dest)
{
    // 1. Initialize dest as an identity matrix
    glm_mat4_identity(dest);

    // 2. Apply Translation (T)
    glm_translate(dest, pos);

    // 3. Convert Quaternion to a mat4 and apply Rotation (R)
    mat4 rot;
    glm_quat_mat4(quat, rot);
    glm_mat4_mul(dest, rot, dest);

    // 4. Apply Scale (S)
    glm_scale(dest, scale);
}

static inline mat4s Sol_Transform(vec3s pos, versors quat, vec3s scale)
{
    mat4s m = glms_mat4_identity();
    m       = glms_translate(m, pos);
    m       = glms_quat_rotate(m, quat);
    m       = glms_scale(m, scale);
    return m;
}

static inline SolTri SolTri_GetWorldSpace(const SolTri *localTri, const versors quat, const vec3s pos, vec3s scale)
{
    // SolTri worldTri    = *localTri;
    // mat4s  modelMatrix = Sol_Transform(pos, quat, scale);

    // worldTri.a = glms_mat4_mulv3(modelMatrix, localTri->a, 1.0f);
    // worldTri.b = glms_mat4_mulv3(modelMatrix, localTri->b, 1.0f);
    // worldTri.c = glms_mat4_mulv3(modelMatrix, localTri->c, 1.0f);

    // // Recalculate normal if non-uniform scale was applied
    // vec3s edge1     = glms_vec3_sub(worldTri.b, worldTri.a);
    // vec3s edge2     = glms_vec3_sub(worldTri.c, worldTri.a);
    // worldTri.normal = glms_vec3_normalize(glms_vec3_cross(edge1, edge2));

    // return worldTri;

    SolTri w;

    // 1. Scale
    vec3s sa = glms_vec3_mul(localTri->a, scale);
    vec3s sb = glms_vec3_mul(localTri->b, scale);
    vec3s sc = glms_vec3_mul(localTri->c, scale);

    // 2. Rotate via Quaternion
    sa = glms_quat_rotatev(quat, sa);
    sb = glms_quat_rotatev(quat, sb);
    sc = glms_quat_rotatev(quat, sc);

    // 3. Translate
    w.a = glms_vec3_add(sa, pos);
    w.b = glms_vec3_add(sb, pos);
    w.c = glms_vec3_add(sc, pos);

    // 4. Recalculate World Normal
    vec3s e1 = glms_vec3_sub(w.b, w.a);
    vec3s e2 = glms_vec3_sub(w.c, w.a);
    w.normal = glms_vec3_normalize(glms_vec3_cross(e1, e2));

    // 5. Recalculate Center and Bounds Radius
    w.center = glms_vec3_scale(glms_vec3_add(glms_vec3_add(w.a, w.b), w.c), 1.0f / 3.0f);

    float da = glms_vec3_norm(glms_vec3_sub(w.a, w.center));
    float db = glms_vec3_norm(glms_vec3_sub(w.b, w.center));
    float dc = glms_vec3_norm(glms_vec3_sub(w.c, w.center));
    w.bounds = fmaxf(da, fmaxf(db, dc));

    return w;
}

static inline int fast_floor(float x)
{
    int i = (int)x;
    return i - (x < i);
}

static inline int clampi(int v, int a, int b)
{
    return v < a ? a : (v > b ? b : v);
}