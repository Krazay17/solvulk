#version 450

// ─── Vertex inputs ──────────────────────────────────────────────────────────
layout(location = 0) in vec3  inPos;
layout(location = 1) in vec3  inNormal;
layout(location = 2) in vec2  inUV;
layout(location = 3) in uvec4 inBoneIndices;
layout(location = 4) in vec4  inBoneWeights;

// ─── Vertex outputs ─────────────────────────────────────────────────────────
layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragWorldPos;
layout(location = 3) flat out int   instanceIndex;
layout(location = 4) flat out uint  flags;
layout(location = 5) flat out float fragHitTime;
layout(location = 6) out vec2 fragUV;

// ─── Descriptors ────────────────────────────────────────────────────────────
layout(set = 0, binding = 0) uniform GameData {
    mat4 dummy;
} game;

layout(set = 1, binding = 0) uniform Scene {
    mat4 viewProjection;
    mat4 view;
    mat4 proj;
    vec4 cameraPos;
    vec4 sun;
} scene;

struct ModelData {
    vec4  position;
    vec4  scale;
    vec4  rotation;
    vec4  color;
    vec4  material;
    uint  flags;
    float hitTime;
    uint  _padding[2];
};

layout(set = 2, binding = 0) readonly buffer Models {
    ModelData models[];
};

#define MAX_BONES 128
struct InstanceSkinning {
    mat4 bones[MAX_BONES];
};

layout(set = 5, binding = 0) readonly buffer Skinning {
    InstanceSkinning skins[];
};

layout(set = 4, binding = 0) uniform Ortho {
    mat4 ortho2d;
};

// ─── Flag bits (mirror these on the CPU) ────────────────────────────────────
const uint FLAG_2D = 1u << 2;

// ─── Quaternion → 3x3 rotation matrix ───────────────────────────────────────
mat3 quatToMat3(vec4 q) {
    float x = q.x, y = q.y, z = q.z, w = q.w;
    float x2 = x + x, y2 = y + y, z2 = z + z;
    float xx = x * x2, xy = x * y2, xz = x * z2;
    float yy = y * y2, yz = y * z2, zz = z * z2;
    float wx = w * x2, wy = w * y2, wz = w * z2;

    return mat3(
        1.0 - (yy + zz), xy + wz,         xz - wy,
        xy - wz,         1.0 - (xx + zz), yz + wx,
        xz + wy,         yz - wx,         1.0 - (xx + yy)
    );
}

void main() {
    ModelData inst = models[gl_InstanceIndex];
    bool is2D = (inst.flags & FLAG_2D) != 0u;

    // ─── Skinning: blend per-vertex bone matrices by their weights ─────────
    mat4 skinMat =
        skins[gl_InstanceIndex].bones[inBoneIndices.x] * inBoneWeights.x +
        skins[gl_InstanceIndex].bones[inBoneIndices.y] * inBoneWeights.y +
        skins[gl_InstanceIndex].bones[inBoneIndices.z] * inBoneWeights.z +
        skins[gl_InstanceIndex].bones[inBoneIndices.w] * inBoneWeights.w;

    vec3 skinnedPos    = (skinMat * vec4(inPos, 1.0)).xyz;
    vec3 skinnedNormal = mat3(skinMat) * inNormal;

    // ─── Instance transform: rotation + scale ──────────────────────────────
    mat3 rot = quatToMat3(inst.rotation);
    vec3 transformedPos    = rot * (skinnedPos * inst.scale.xyz);
    vec3 transformedNormal = rot * normalize(skinnedNormal);

    // ─── 2D Y-flip ─────────────────────────────────────────────────────────
    // 3D models are authored Y-up. ortho2d treats input Y as screen-down
    // (Vulkan NDC). Flip vertically so the model isn't upside-down.
    // Flip BOTH position and normal so surface lighting stays correct on the
    // mirrored geometry.
    if (is2D) {
        transformedPos.y    = -transformedPos.y;
        transformedNormal.y = -transformedNormal.y;
    }

    // ─── Translate into world / UI space ───────────────────────────────────
    vec3 worldPos = transformedPos + inst.position.xyz;

    // ─── Project ───────────────────────────────────────────────────────────
    // 2D: project XY through ortho and force Z = 0. The model's natural Z
    //     extent (chest depth, etc.) would otherwise map outside Vulkan's
    //     NDC.z [0, 1] via the OpenGL-convention ortho and get depth-clipped.
    // 3D: standard view-projection.
    gl_Position = is2D
        ? ortho2d * vec4(worldPos.xy, 0.5 + worldPos.z * 0.001, 1.0)
        : scene.viewProjection * vec4(worldPos, 1.0);

    // ─── Pass-throughs ─────────────────────────────────────────────────────
    fragWorldPos  = worldPos;
    fragNormal    = transformedNormal;
    fragColor     = inst.color;
    fragUV        = inUV;
    instanceIndex = gl_InstanceIndex;
    flags         = inst.flags;
    fragHitTime   = inst.hitTime;
}