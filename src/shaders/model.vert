#version 450

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragMaterial;
layout(location = 3) out vec3 fragWorldPos;
layout(location = 4) flat out int outInstanceIndex;


layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(set = 0, binding = 0) uniform Scene {
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
};

layout(std430, set = 1, binding = 0) readonly buffer ModelBuffer {
    ModelData instances[];
};

mat3 quatToMat3(vec4 q) {
    float x = q.x, y = q.y, z = q.z, w = q.w;
    float x2 = x + x, y2 = y + y, z2 = z + z;
    float xx = x * x2, xy = x * y2, xz = x * z2;
    float yy = y * y2, yz = y * z2, zz = z * z2;
    float wx = w * x2, wy = w * y2, wz = w * z2;

    return mat3(
        1.0 - (yy + zz), xy + wz, xz - wy,
        xy - wz, 1.0 - (xx + zz), yz + wx,
        xz + wy, yz - wx, 1.0 - (xx + yy)
    );
}

void main() {
    ModelData inst = instances[gl_InstanceIndex];

    mat3 mat = quatToMat3(inst.rotation);

    vec3 worldPos = mat * (inPos * inst.scale.xyz) + inst.position.xyz;
    gl_Position = scene.viewProj * vec4(worldPos, 1.0);

    fragColor = inst.color;
    fragNormal = mat * inNormal;
    fragMaterial = inst.material.xy;
    fragWorldPos = worldPos;
    outInstanceIndex = gl_InstanceIndex;
}