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

void main() {
    ModelData inst = instances[gl_InstanceIndex];
    instanceIndex = gl_InstanceIndex;
    flags = inst.flags;
    fragHitTime = inst.hitTime;
    fragColor = inst.color;
    fragUV = inUV;
    mat3 rotMat = quatToMat3(inst.rotation);

    // Check if the 2D flag bit is set
    if ((inst.flags & FLAG_2D) != 0u)
        {
        // 1. Scale local vertex positions
        vec3 scaledPos = inPos * inst.scale.xyz;
    
        // 2. Rotate mesh in 2D/3D space
        vec3 rotatedPos = rotMat * scaledPos;
        vec3 rotatedNormal = rotMat * inNormal;

        // 3. FLIP LOCAL Y (3D models authored Y-up vs. Top-Down Ortho Space)
        rotatedPos.y    = -rotatedPos.y;
        rotatedNormal.y = -rotatedNormal.y;
    
        // 4. Translate to screen-space pixel position (X, Y) + Z layer
        vec3 uiWorldPos = rotatedPos + inst.position.xyz;
    
        // 5. Transform via orthographic projection & fix Z layer clipping
        gl_Position = ortho2d * vec4(uiWorldPos.xy, 0.5 + uiWorldPos.z * 0.001, 1.0);
    
        fragWorldPos = uiWorldPos;
        fragNormal   = rotatedNormal;
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