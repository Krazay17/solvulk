#include "sol_core.h"

const vec3s VECTOR_RADIAL_DIRECTIONS[4] = {
    { 1.0f, 0.0f,  0.0f}, // Forward
    { 0.0f, 0.0f,  1.0f}, // Right
    {-1.0f, 0.0f,  0.0f}, // Backward
    { 0.0f, 0.0f, -1.0f}  // Left
};

bool Sol_Check_2d_Collision(vec2s a, vec4s b)
{
    return !((a.x < b.x) | (a.x >= b.x + b.z) | (a.y < b.y) | (a.y >= b.y + b.w));
}

SolVec3 Sol_Vec3_Add(SolVec3 a, SolVec3 b)
{
    return (SolVec3){a.x + b.x, a.y + b.y, a.z + b.z};
}

float Sol_Lerp(float start, float end, float amount)
{
    return start + amount * (end - start);
}

vec4s Sol_Color_Lerp(vec4s base, vec4s target, float alpha)
{
    float r = base.r + alpha * (target.r - base.r);
    float g = base.g + alpha * (target.g - base.g);
    float b = base.b + alpha * (target.b - base.b);
    float a = base.a + alpha * (target.a - base.a);
    return (vec4s){
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
    float yaw   = atan2f(lookDir.x, lookDir.z);
    float pitch = asinf(lookDir.y);

    return Sol_Quat_FromYawPitch(yaw, -pitch);
}

versors Sol_Quat_FromLookDira(vec3s lookDir)
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

float Sol_YawFromQuat(versor q)
{
    return atan2f(2.0f * (q[1] * q[2] + q[3] * q[0]), q[3] * q[3] - q[0] * q[0] - q[1] * q[1] + q[2] * q[2]);
}
float PulseAnim(float dt, float value, float speed)
{
    value += dt * speed;
    value = (sinf(value) * 0.5f) + 0.5f;
    return value;
}
