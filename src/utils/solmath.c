#include "solmath.h"
#include <cglm/struct.h>
#include <cglm/cglm.h>

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
vec4s Sol_Quat_FromYawPitch(float yaw, float pitch)
{
    versor q;
    glm_quat_identity(q);

    // Create quats for each axis
    versor q_yaw, q_pitch;
    glm_quatv(q_yaw, yaw, (vec3){0.0f, 1.0f, 0.0f});     // Y-Axis
    glm_quatv(q_pitch, pitch, (vec3){1.0f, 0.0f, 0.0f}); // X-Axis

    // Combine them: q = q_yaw * q_pitch
    glm_quat_mul(q_yaw, q_pitch, q);

    return (vec4s){
        q[0],
        q[1],
        q[2],
        q[3],
    };
}