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

layout(set = 3, binding = 0) uniform Ortho {mat4 ortho2d;};

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

// void main() {
//     ModelData inst = instances[gl_InstanceIndex];

//     mat3 mat = quatToMat3(inst.rotation);

//     vec3 worldPos = mat * (inPos * inst.scale.xyz) + inst.position.xyz;
//     gl_Position = scene.viewProj * vec4(worldPos, 1.0);

//     fragColor = inst.color;
//     fragNormal = mat * inNormal;
//     fragWorldPos = worldPos;
//     flags = inst.flags;
//     fragHitTime = inst.hitTime;
//     instanceIndex = gl_InstanceIndex;
// }
void main() {
    ModelData inst = instances[gl_InstanceIndex];
    instanceIndex = gl_InstanceIndex;
    flags = inst.flags;
    fragHitTime = inst.hitTime;
    fragColor = inst.color;

    // Check if the 2D flag bit is set
    if ((inst.flags & FLAG_2D) != 0u) 
    {
        // 1. Compute 2D position using local inPos vertices, scale, and translation offset
        // Typically, UI meshes are flat unit quads on the XY plane (Z=0)
        vec2 localScaled = inPos.xy * inst.scale.xy;
        vec2 uiWorldPos  = localScaled + inst.position.xy;

        // 2. Project directly using your 2D Orthographic Screen Matrix
        // We set Z slightly via inst.position.z or use a constant to handle UI layers (0.0 to 1.0)
        gl_Position = ortho2d * vec4(uiWorldPos, inst.position.z, 1.0);

        // 3. Passthroughs for UI/Text (Normals aren't used for 2D, but don't leave them uninitialized)
        fragWorldPos = vec3(uiWorldPos, inst.position.z);
        fragNormal   = vec3(0.0, 0.0, 1.0); 
    } 
    else 
    {
        // --- Standard 3D Render Path ---
        mat3 mat = quatToMat3(inst.rotation);

        vec3 worldPos = mat * (inPos * inst.scale.xyz) + inst.position.xyz;
        gl_Position   = scene.viewProj * vec4(worldPos, 1.0);

        fragWorldPos = worldPos;
        fragNormal   = mat * inNormal;
    }
}