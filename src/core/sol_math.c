#include <cglm/cglm.h>
#include <cglm/struct.h>

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
    {-0.7071f, 0.0f, 1.0f},     // South-West
    {-0.7071f, 0.0f, -0.7071f}, // North-West
};

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
