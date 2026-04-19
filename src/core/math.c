#include <cglm/struct.h>

#include "sol_core.h"

vec3s ClosestPointOnTriangle(vec3s p, vec3s a, vec3s b, vec3s c)
{
    vec3s ab = glms_vec3_sub(b, a);
    vec3s ac = glms_vec3_sub(c, a);
    vec3s ap = glms_vec3_sub(p, a);

    float d1 = glms_vec3_dot(ab, ap);
    float d2 = glms_vec3_dot(ac, ap);
    if (d1 <= 0.0f && d2 <= 0.0f)
        return a;

    vec3s bp = glms_vec3_sub(p, b);
    float d3 = glms_vec3_dot(ab, bp);
    float d4 = glms_vec3_dot(ac, bp);
    if (d3 >= 0.0f && d4 <= d3)
        return b;

    vec3s cp = glms_vec3_sub(p, c);
    float d5 = glms_vec3_dot(ab, cp);
    float d6 = glms_vec3_dot(ac, cp);
    if (d6 >= 0.0f && d5 <= d6)
        return c;

    float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
    {
        float v = d1 / (d1 - d3);
        return glms_vec3_add(a, glms_vec3_scale(ab, v));
    }

    float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
    {
        float w = d2 / (d2 - d6);
        return glms_vec3_add(a, glms_vec3_scale(ac, w));
    }

    float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f)
    {
        float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        return glms_vec3_add(b, glms_vec3_scale(glms_vec3_sub(c, b), w));
    }

    float denom = va + vb + vc;
    if (denom <= 1e-8f)
        return a;

    float inv = 1.0f / denom;
    float v = vb * inv;
    float w = vc * inv;
    return glms_vec3_add(a, glms_vec3_add(glms_vec3_scale(ab, v), glms_vec3_scale(ac, w)));
}

bool Sol_Check_2d_Collision(vec2s a, vec4s b)
{
    return !((a.x < b.x) |
             (a.x >= b.x + b.z) |
             (a.y < b.y) |
             (a.y >= b.y + b.w));
}

SolVec3 Sol_Vec3_Add(SolVec3 a, SolVec3 b)
{
    return (SolVec3){a.x + b.x,
                     a.y + b.y,
                     a.z + b.z};
}

float Sol_Lerp(float start, float end, float amount)
{
    return start + amount * (end - start);
}

SolColor Sol_Color_Lerp(SolColor base, SolColor target, float alpha)
{
    float r = base.r + alpha * (target.r - base.r);
    float g = base.g + alpha * (target.g - base.g);
    float b = base.b + alpha * (target.b - base.b);
    float a = base.a + alpha * (target.a - base.a);
    return (SolColor){
        .r = (uint8_t)fminf(r, 255),
        .g = (uint8_t)fminf(g, 255),
        .b = (uint8_t)fminf(b, 255),
        .a = (uint8_t)fminf(a, 255),
    };
}

vec3s Sol_Vec3_FromYawPitch(float yaw, float pitch)
{
    float x = cosf(pitch) * sinf(yaw);
    float y = sinf(pitch);
    float z = cosf(pitch) * cosf(yaw);
    return (vec3s){x, y, z};
}

// In your Movement System or Controller
versors Sol_Quat_FromYawPitch(float yaw, float pitch)
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

versors Sol_Quat_FromLookDir(vec3s lookDir)
{
    // Flatten to horizontal for yaw, then get pitch from vertical component
    float yaw = atan2f(lookDir.x, lookDir.z);
    float pitch = asinf(lookDir.y);

    return Sol_Quat_FromYawPitch(yaw, -pitch);
}

versors Sol_Quat_FromLookDira(vec3s lookDir)
{
    vec3s forward = {0.0f, 0.0f, 1.0f};
    vec3s dir = glms_vec3_normalize(lookDir);

    float dot = glms_vec3_dot(forward, dir);

    // Nearly the same direction
    if (dot > 0.9999f)
        return (versors){0, 0, 0, 1};

    // Nearly opposite
    if (dot < -0.9999f)
        return (versors){0, 1, 0, 0}; // 180° around Y

    vec3s axis = glms_vec3_normalize(glms_vec3_cross(forward, dir));
    float angle = acosf(dot);

    versor q;
    glm_quatv(q, angle, (vec3){axis.x, axis.y, axis.z});

    return (versors){q[0], q[1], q[2], q[3]};
}

float FlashAnim(float dt, float value, float speed)
{
    speed = (speed > 0.0f) ? speed : 4.0f;
    value -= dt * speed;
    if (value < 0.0f)
        value = 0.0f;
    return value;
}

float PulseAnim(float dt, float value, float speed)
{
    speed = (speed > 0.0f) ? speed : 4.0f;
    value += dt * speed;
    value = (sinf(value) * 0.5f) + 0.5f;
    return value;
}
