#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragWorldPos;
layout(location = 3) flat out int instanceIndex;
layout(location = 4) flat out uint flags;
layout(location = 5) flat out float fragHitTime;
layout(location = 6) out vec2 fragUV;

layout(set = 1, binding = 0) uniform Scene {
    mat4 viewProj;
    mat4 view;
    mat4 proj;
    vec4 cameraPos;
    vec4 sun;
} scene;

struct ModelData {
    vec4 position;
    vec4 scale;
    vec4 rotation;
    vec4 color;
    vec4 material;
    uint flags;
    float hitTime;
    uint _padding[2];
};

const uint FLAG_2D = 1u << 2;

layout(set = 2, binding = 0) readonly buffer ModelBuffer {
    ModelData instances[];
};

layout(set = 3, binding = 0) uniform Ortho {
    mat4 ortho2d;
};

mat3 quatToMat3(vec4 q)
{
    float x = q.x;
    float y = q.y;
    float z = q.z;
    float w = q.w;

    float x2 = x + x;
    float y2 = y + y;
    float z2 = z + z;

    float xx = x * x2;
    float xy = x * y2;
    float xz = x * z2;

    float yy = y * y2;
    float yz = y * z2;
    float zz = z * z2;

    float wx = w * x2;
    float wy = w * y2;
    float wz = w * z2;

    return mat3(
        1.0 - (yy + zz), xy + wz,         xz - wy,
        xy - wz,         1.0 - (xx + zz), yz + wx,
        xz + wy,         yz - wx,         1.0 - (xx + yy)
    );
}

void main()
{
    ModelData inst = instances[gl_InstanceIndex];

    instanceIndex = gl_InstanceIndex;
    flags         = inst.flags;
    fragHitTime   = inst.hitTime;
    fragColor     = inst.color;
    fragUV        = inUV;

    mat3 rotation = quatToMat3(inst.rotation);

    // Models are authored:
    //   +Y = up
    //   +Z = forward
    //   +X = left
    //
    // No model-axis correction is performed here.
    vec3 localPos = inPos * inst.scale.xyz;
    vec3 localNormal = inNormal;

    vec3 rotatedPos = rotation * localPos;
    vec3 rotatedNormal = rotation * localNormal;

    vec3 worldPos = rotatedPos + inst.position.xyz;

    if ((inst.flags & FLAG_2D) != 0u)
    {
        // This is a UI/projection-space conversion, NOT a model
        // orientation correction.
        worldPos.y = -worldPos.y;
        rotatedNormal.y = -rotatedNormal.y;

        gl_Position =
            ortho2d *
            vec4(
                worldPos.xy,
                0.5 + worldPos.z * 0.001,
                1.0
            );

        fragWorldPos = worldPos;
        fragNormal   = rotatedNormal;
    }
    else
    {
        gl_Position =  scene.viewProj * vec4(worldPos, 1.0);

        fragWorldPos = worldPos;
        fragNormal   = rotatedNormal;
    }
}