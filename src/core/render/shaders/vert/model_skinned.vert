#version 450

layout(location = 0) in vec3  inPos;
layout(location = 1) in vec3  inNormal;
layout(location = 2) in vec2  inUV;
layout(location = 3) in uvec4 inBoneIndices;
layout(location = 4) in vec4  inBoneWeights;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragWorldPos;
layout(location = 3) flat out int   instanceIndex;
layout(location = 4) flat out uint  flags;
layout(location = 5) flat out float fragHitTime;
layout(location = 6) out vec2 fragUV;

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
    vec4 position;
    vec4 scale;
    vec4 rotation;
    vec4 color;
    vec4 material;
    uint flags;
    float hitTime;
    uint _padding[2];
};

layout(set = 2, binding = 0) readonly buffer Models {
    ModelData models[];
};

layout(set = 3, binding = 0) uniform Ortho {
    mat4 ortho2d;
};

#define MAX_BONES 128

struct InstanceSkinning {
    mat4 bones[MAX_BONES];
};

layout(set = 5, binding = 0) readonly buffer Skinning {
    InstanceSkinning skins[];
};

const uint FLAG_2D = 1u << 2;

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
    ModelData inst = models[gl_InstanceIndex];

    bool is2D = (inst.flags & FLAG_2D) != 0u;

    // -------------------------------------------------------------------------
    // Skinning
    //
    // Bone matrices operate entirely in model space.
    // No coordinate-system conversion happens here.
    // -------------------------------------------------------------------------

    mat4 skinMat =
        skins[gl_InstanceIndex].bones[inBoneIndices.x] * inBoneWeights.x +
        skins[gl_InstanceIndex].bones[inBoneIndices.y] * inBoneWeights.y +
        skins[gl_InstanceIndex].bones[inBoneIndices.z] * inBoneWeights.z +
        skins[gl_InstanceIndex].bones[inBoneIndices.w] * inBoneWeights.w;

    vec3 skinnedPos =
        (skinMat * vec4(inPos, 1.0)).xyz;

    vec3 skinnedNormal =
        mat3(skinMat) * inNormal;

    // -------------------------------------------------------------------------
    // Entity transform
    //
    // Asset convention:
    //   +Y = up
    //   +Z = forward
    //   +X = left
    //
    // inst.rotation is the actual entity/world rotation.
    // -------------------------------------------------------------------------

    mat3 rotation = quatToMat3(inst.rotation);

    vec3 transformedPos =
        rotation * (skinnedPos * inst.scale.xyz);

    vec3 transformedNormal =
        rotation * normalize(skinnedNormal);

    vec3 worldPos =
        transformedPos + inst.position.xyz;

    // -------------------------------------------------------------------------
    // 2D projection
    //
    // This Y flip is only a conversion into the UI coordinate system.
    // It is NOT correcting the model's orientation.
    // -------------------------------------------------------------------------

    if (is2D)
    {
        worldPos.y = -worldPos.y;
        transformedNormal.y = -transformedNormal.y;

        gl_Position =
            ortho2d *
            vec4(
                worldPos.xy,
                0.5 + worldPos.z * 0.001,
                1.0
            );
    }
    else
    {
        gl_Position =
            scene.viewProjection *
            vec4(worldPos, 1.0);
    }

    fragWorldPos  = worldPos;
    fragNormal    = transformedNormal;
    fragColor     = inst.color;
    fragUV        = inUV;
    instanceIndex = gl_InstanceIndex;
    flags         = inst.flags;
    fragHitTime   = inst.hitTime;
}