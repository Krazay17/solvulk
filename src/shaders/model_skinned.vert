#version 450

layout(location = 0) in vec3  inPos;
layout(location = 1) in vec3  inNormal;
layout(location = 2) in vec2  inUv;
layout(location = 3) in uvec4 inBoneIndices;
layout(location = 4) in vec4  inBoneWeights;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragWorldPos;
layout(location = 3) flat out int instanceIndex;

// UBO
layout(set = 0, binding = 0) uniform Scene {
    mat4 viewProjection;
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
    vec4 material;};
layout(set = 1, binding = 0) readonly buffer Models {
    ModelData models[];};

#define MAX_BONES 128
struct InstanceSkinning {
    mat4 bones[MAX_BONES];};
layout(set = 3, binding = 0) readonly buffer Skinning {
    InstanceSkinning skins[];};

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
    ModelData inst = models[gl_InstanceIndex];
    
    // Blend bone matrices by weights
    mat4 skinMat = skins[gl_InstanceIndex].bones[inBoneIndices.x] * inBoneWeights.x +
                   skins[gl_InstanceIndex].bones[inBoneIndices.y] * inBoneWeights.y +
                   skins[gl_InstanceIndex].bones[inBoneIndices.z] * inBoneWeights.z +
                   skins[gl_InstanceIndex].bones[inBoneIndices.w] * inBoneWeights.w;
    
    // Local-space → animated local-space → instance-transformed → clip
    vec4 skinnedPos    = skinMat * vec4(inPos, 1.0);
    vec3 skinnedNormal = mat3(skinMat) * inNormal;
    
    mat3 rot = quatToMat3(inst.rotation);
    vec3 worldPos = rot * (skinnedPos.xyz * inst.scale.xyz) + inst.position.xyz;
    
    fragWorldPos = worldPos;
    fragNormal   = rot * normalize(skinnedNormal);
    fragColor    = inst.color;
    instanceIndex = gl_InstanceIndex;
    
    gl_Position = scene.viewProjection * vec4(worldPos, 1.0);
    
}