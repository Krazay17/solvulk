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
#define vecLen(a) glms_vec3_norm((a))
#define vecNorm(a) glms_vec3_normalize(a)
#define vecCross(a, b) glms_vec3_cross((a), (b))
#define vecLerp(a, b, c) glms_vec3_lerp(a, b, c)
#define vecDist(a, b) glms_vec3_distance(a, b)

#define SOL_COLOR(hex) (vec4s){.r = ((hex) >> 16) & 0xFF, .g = ((hex) >> 8) & 0xFF, .b = ((hex)) & 0xFF, .a = 255}

#define SOL_COLORA(hex, alpha)                                                                                         \
    (vec4s)                                                                                                            \
    {                                                                                                                  \
        .r = ((hex) >> 16) & 0xFF, .g = ((hex) >> 8) & 0xFF, .b = ((hex)) & 0xFF, .a = (alpha)                         \
    }

extern const vec3s VECTOR_RADIAL_DIRECTIONS[9];
extern const vec4s SPRITEPAGE4[4];

#ifdef SOL_MATH_IMPLEMENTATION
const vec3s VECTOR_RADIAL_DIRECTIONS[9] = {
    // Cardinal Directions
    {0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 1.0f},  // South / Forward
    {0.0f, 0.0f, -1.0f}, // North / Backward
    {1.0f, 0.0f, 0.0f},  // East / Right
    {-1.0f, 0.0f, 0.0f}, // West / Left

    // Diagonal Directions
    {0.7071f, 0.0f, 0.7071f},   // South-East
    {0.7071f, 0.0f, -0.7071f},  // North-East
    {-0.7071f, 0.0f, 0.7071f},  // South-West
    {-0.7071f, 0.0f, -0.7071f}, // North-West
};
const vec4s SPRITEPAGE4[4] = {
    {0.5f, 0.5f, 0.0f, 0.0f},
    {0.5f, 0.5f, 0.5f, 0.0f},
    {0.5f, 0.5f, 0.0f, 0.5f},
    {0.5f, 0.5f, 0.5f, 0.5f},
};
#endif

// INLINES-------------------

static inline float maxf(float a, float b)
{
    return (a > b) ? a : b;
}
static inline float minf(float a, float b)
{
    return (a < b) ? a : b;
}
static inline versors to_versors(vec4s v)
{
    return *(versors *)&v;
}
static inline vec4s to_vec4s(versors v)
{
    return *(vec4s *)&v;
}
static inline vec3s Sol_Vec3_FromYawPitch(float yaw, float pitch)
{
    float x = cosf(pitch) * sinf(yaw);
    float y = sinf(pitch);
    float z = cosf(pitch) * cosf(yaw);
    return (vec3s){x, y, z};
}

static inline versors Sol_Quat_FromYawPitch(float yaw, float pitch)
{
    versor q;
    glm_quat_identity(q);

    // Create quats for each axis
    versor q_yaw, q_pitch;
    glm_quatv(q_yaw, yaw, (vec3){0.0f, 1.0f, 0.0f});     // Y-Axis
    glm_quatv(q_pitch, pitch, (vec3){1.0f, 0.0f, 0.0f}); // X-Axis

    // Combine them: q = q_yaw * q_pitch
    glm_quat_mul(q_yaw, q_pitch, q);

    return (versors){
        q[0],
        q[1],
        q[2],
        q[3],
    };
}

static inline vec4s Sol_Rot_FromVecs(vec3s fwd, vec3s up)
{
    vec3s fwd_dir = glms_vec3_normalize(fwd);

    // Guard against singularity when looking straight along the UP axis
    if (fabsf(glms_vec3_dot(fwd_dir, up)) > 0.999f)
    {
        up = (vec3s){0.0f, 0.0f, 1.0f};
    }

    vec3s right = glms_vec3_normalize(glms_vec3_cross(fwd_dir, up));
    vec3s dirZ  = glms_vec3_cross(right, fwd_dir); // Already unit length!

    mat3s rot_mat = {.col[0] = dirZ, .col[1] = fwd_dir, .col[2] = glms_vec3_negate(right)};

    versors stable_rot = glms_mat3_quat(rot_mat);
    return (vec4s){stable_rot.x, stable_rot.y, stable_rot.z, stable_rot.w};
}

static inline versors Sol_Quat_FromLookDir(vec3s lookDir)
{
    // Flatten to horizontal for yaw, then get pitch from vertical component
    float yaw   = atan2f(lookDir.x, lookDir.z);
    float pitch = asinf(lookDir.y);

    return Sol_Quat_FromYawPitch(yaw, -pitch);
}

static inline versors Sol_Quat_FromLookDira(vec3s lookDir)
{
    vec3s forward = {0.0f, 0.0f, 1.0f};
    vec3s dir     = glms_vec3_normalize(lookDir);

    float dot = glms_vec3_dot(forward, dir);

    // Nearly the same direction
    if (dot > 0.9999f)
        return (versors){0, 0, 0, 1};

    // Nearly opposite
    if (dot < -0.9999f)
        return (versors){0, 1, 0, 0}; // 180° around Y

    vec3s axis  = glms_vec3_normalize(glms_vec3_cross(forward, dir));
    float angle = acosf(dot);

    versor q;
    glm_quatv(q, angle, (vec3){axis.x, axis.y, axis.z});

    return (versors){q[0], q[1], q[2], q[3]};
}

static inline AngleSector Sol_GetAngleSector(float dot)
{
    return (AngleSector)((int)floorf(dot * 7) & 7);
}

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
    if (denom < FLOAT_EPSILON)
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
    // Index mapping: q[0] = x, q[1] = y, q[2] = z, q[3] = w
    float siny_cosp = 2.0f * (q[3] * q[1] - q[0] * q[2]);
    float cosy_cosp = q[3] * q[3] + q[0] * q[0] - q[1] * q[1] - q[2] * q[2];

    return atan2f(siny_cosp, cosy_cosp);
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

static inline vec3s Sol_RotFromQuat(versors quat, vec3s axis)
{
    mat4s mat = glms_quat_mat4(quat);
    return glms_mat4_mulv3(mat, axis, 1.0f);
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

static inline versors Sol_VelToQuat(vec3s dir)
{
    float v2 = glms_vec3_norm2(dir);
    if (v2 > 0.001f)
    {
        versors quat   = GLMS_QUAT_IDENTITY;
        vec3s dir      = glms_vec3_scale(dir, 1.0f / sqrt(v2));
        vec3s base_dir = {0.0f, 1.0f, 0.0f};
        return glms_quat_from_vecs(base_dir, dir);
    }
    return GLMS_QUAT_IDENTITY;
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

static inline void ClosestTSegmentSegment(vec3s a0, vec3s a1, // Segment A: a0 -> a1
                                          vec3s b0, vec3s b1, // Segment B: b0 -> b1
                                          float *outTA, float *outTB)
{
    vec3s dA = glms_vec3_sub(a1, a0); // Direction of Segment A
    vec3s dB = glms_vec3_sub(b1, b0); // Direction of Segment B
    vec3s r  = glms_vec3_sub(a0, b0); // Displacement between starts

    float lenSqA = glms_vec3_dot(dA, dA); // Squared length of Segment A
    float lenSqB = glms_vec3_dot(dB, dB); // Squared length of Segment B
    float f      = glms_vec3_dot(dB, r);

    float tA, tB;

    // Both segments degenerate to points?
    if (lenSqA <= FLOAT_EPSILON && lenSqB <= FLOAT_EPSILON)
    {
        *outTA = 0.0f;
        *outTB = 0.0f;
        return;
    }

    if (lenSqA <= FLOAT_EPSILON)
    {
        // Segment A is a point
        tA = 0.0f;
        tB = fmaxf(0.0f, fminf(1.0f, f / lenSqB));
    }
    else
    {
        float c = glms_vec3_dot(dA, r);

        if (lenSqB <= FLOAT_EPSILON)
        {
            // Segment B is a point
            tB = 0.0f;
            tA = fmaxf(0.0f, fminf(1.0f, -c / lenSqA));
        }
        else
        {
            // General non-parallel case
            float b     = glms_vec3_dot(dA, dB);
            float denom = lenSqA * lenSqB - b * b;

            if (denom != 0.0f)
            {
                tA = fmaxf(0.0f, fminf(1.0f, (b * f - c * lenSqB) / denom));
            }
            else
            {
                // Parallel — pick tA = 0 arbitrarily
                tA = 0.0f;
            }

            tB = (b * tA + f) / lenSqB;

            // Clamp tB and recompute tA if needed
            if (tB < 0.0f)
            {
                tB = 0.0f;
                tA = fmaxf(0.0f, fminf(1.0f, -c / lenSqA));
            }
            else if (tB > 1.0f)
            {
                tB = 1.0f;
                tA = fmaxf(0.0f, fminf(1.0f, (b - c) / lenSqA));
            }
        }
    }

    *outTA = tA; // Progress along Segment A [0, 1]
    *outTB = tB; // Progress along Segment B [0, 1]
}

static inline void ClosestPointsSegmentSegment(vec3s a0, vec3s a1, vec3s b0, vec3s b1, vec3s *outPointA,
                                               vec3s *outPointB)
{
    float tA, tB;
    ClosestTSegmentSegment(a0, a1, b0, b1, &tA, &tB);

    vec3s dA = glms_vec3_sub(a1, a0);
    vec3s dB = glms_vec3_sub(b1, b0);

    *outPointA = glms_vec3_add(a0, glms_vec3_scale(dA, tA));
    *outPointB = glms_vec3_add(b0, glms_vec3_scale(dB, tB));
}

static inline float ClosestTOnSegment(vec3s s0, vec3s s1, vec3s p)
{
    vec3s m  = glms_vec3_sub(s1, s0);
    float d2 = glms_vec3_dot(m, m);
    if (d2 < FLOAT_EPSILON)
        return 0.0f;
    float t = glms_vec3_dot(glms_vec3_sub(p, s0), m) / d2;
    return maxf(0.0f, minf(1.0f, t));
}
static inline vec3s ClosestPointOnSegment(vec3s s0, vec3s s1, vec3s p)
{
    float t = ClosestTOnSegment(s0, s1, p);
    return glms_vec3_add(s0, glms_vec3_scale(glms_vec3_sub(s1, s0), t));
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

static inline vec3s Sol_AddScaledDir(vec3s start, vec3s dir, float dist)
{
    return glms_vec3_add(start, glms_vec3_scale(dir, dist));
}

typedef struct
{
    int cols;
    vec2s start;
    vec2s spacing;

    int _counter;
} GridMaker;
static inline vec3s Sol_Grid_Next(GridMaker *grid)
{
    int c       = grid->_counter++;
    int col_idx = c % grid->cols;
    int row_idx = c / grid->cols;
    return (vec3s){grid->start.x + (float)col_idx * grid->spacing.x, grid->start.y + (float)row_idx * grid->spacing.y,
                   0};
}

typedef enum
{
    CURVE_CONSTANT, // Always 1.0
    CURVE_SCURVEY,
    CURVE_LATEPULSE,
    CURVE_LINEAR_FADEIN,   // 0 -> 1
    CURVE_LINEAR_FADEOUT,  // 1 -> 0
    CURVE_SMOOTH_INOUT,    // Symmetric Arch (0 -> 1 -> 0, peaks at t=0.5)
    CURVE_QUICKIN_SLOWOUT, // Flash Arch (0 -> 1 -> 0, peaks fast at t=0.33)
    CURVE_EASE_IN,         // Quadratic Ease In (0 -> 1)
    CURVE_EASE_OUT,        // Quadratic Ease Out (0 -> 1)
    CURVE_SMOOTHSTEP,      // Hermite Smoothstep (0 -> 1)
    CURVE_COUNT,
} CurveTypes;

static inline float EvaluateCurve(CurveTypes type, float t)
{
    switch (type)
    {
    case CURVE_LATEPULSE: {
        // float c = 149.01161f;
        // float a = powf(t, 8.0f);
        // float b = powf(1.0f - t, 2.0f);
        // float z = c * a * b;
        // return z;

        // Exponents
        const float p = 8.0f;
        const float q = 2.0f;

        // Peak location: p / (p + q) = 8 / 10 = 0.8
        const float t_max = p / (p + q);

        // Unscaled value at current t and at peak
        float f_t   = powf(t, p) * powf(1.0f - t, q);
        float f_max = powf(t_max, p) * powf(1.0f - t_max, q);

        return f_t / f_max;
    }
    case CURVE_SCURVEY: {

        float a = powf(t, 8.0f);
        float b = powf(1.0f - t, 1.5f);
        return a / (a + b);
    }
    case CURVE_LINEAR_FADEIN:
        return t;

    case CURVE_LINEAR_FADEOUT:
        return 1.0f - t;

    case CURVE_SMOOTH_INOUT:
        // Parabolic arch: 4t(1-t)
        return 4.0f * t * (1.0f - t);

    case CURVE_QUICKIN_SLOWOUT:
        // Asymmetric arch: 6.75 * t * (1-t)^2
        return 6.75f * t * (1.0f - t) * (1.0f - t);

    case CURVE_EASE_IN:
        // Slow start, fast finish: t^2
        return t * t;

    case CURVE_EASE_OUT:
        // Fast start, slow finish: t(2-t)
        return t * (2.0f - t);

    case CURVE_SMOOTHSTEP:
        // S-Curve transition: 3t^2 - 2t^3
        return t * t * (3.0f - 2.0f * t);

    case CURVE_CONSTANT:
    default:
        return 1.0f;
    }
}