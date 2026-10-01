#version 450

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec4 outColor;
layout(location = 3) flat out uint fragTextureId;

struct RibbonSeg {
    vec4 posA;       // .xyz = World Pos A, .w = Half Width A
    vec4 posB;       // .xyz = World Pos B, .w = Half Width B
    vec4 colorA;     // RGBA at endpoint A
    vec4 colorB;     // RGBA at endpoint B
    vec4 uv;         // .xy = Offset/Pan (U, V), .zw = Scale/Tile (U, V)
    uint textureId;  // Texture array/bindless index
    uint flags;      // 0 = Face Camera (Default), 1u = Align World Up
    float panSpeed;  // U-axis scroll speed per second
    uint _pad;       // Maintains 96-byte std430 alignment
};

layout(set = 0, binding = 0) uniform Game {
    double gameTime;
};

layout(set = 1, binding = 0) uniform Scene {
    mat4 viewProj;
    mat4 view;
    mat4 proj;
    vec4 cameraPos;
    vec4 sun;
    float aspect;
};

layout(set = 2, binding = 0) readonly buffer RibbonSSBO {
    RibbonSeg segs[];
};

const vec2 CORNERS[6] = vec2[](
    vec2(-1.0, 0.0), // Left  A
    vec2( 1.0, 0.0), // Right A
    vec2(-1.0, 1.0), // Left  B
    vec2( 1.0, 0.0), // Right A
    vec2( 1.0, 1.0), // Right B
    vec2(-1.0, 1.0)  // Left  B
);

void main()
{
    RibbonSeg seg = segs[gl_InstanceIndex];
    vec2 corner   = CORNERS[gl_VertexIndex];

    float sideSign = corner.x; // -1.0 or +1.0
    float tSeg     = corner.y; //  0.0 or  1.0

    // Interpolate centerline position and width
    vec3  posA    = seg.posA.xyz;
    vec3  posB    = seg.posB.xyz;
    vec3  basePos = mix(posA, posB, tSeg);
    float halfW   = mix(seg.posA.w, seg.posB.w, tSeg);

    // Segment orientation vector
    vec3 segDir = posB - posA;
    float segLen = length(segDir);
    segDir = (segLen > 0.0001) ? segDir / segLen : vec3(0.0, 1.0, 0.0);

    // Extrusion direction: DEFAULT (0) = Face Camera, OPT-IN (1u) = World Up
    vec3 sideDir;
    bool alignWorldUp = (seg.flags & 1u) != 0u;

    if (alignWorldUp) {
        vec3 up = (abs(segDir.y) > 0.99) ? vec3(0.0, 0.0, 1.0) : vec3(0.0, 1.0, 0.0);
        sideDir = normalize(cross(segDir, up));
    } else {
        vec3 viewDir = normalize(cameraPos.xyz - basePos);
        sideDir = cross(segDir, viewDir);
        float lenSq = dot(sideDir, sideDir);
        sideDir = (lenSq > 0.0001) ? normalize(sideDir) : vec3(1.0, 0.0, 0.0);
    }

    // World space position & clip space transform
    vec3 worldPos = basePos + sideDir * (sideSign * halfW);
    gl_Position   = viewProj * vec4(worldPos, 1.0);

    // Color & Texture ID
    outColor      = mix(seg.colorA, seg.colorB, tSeg);
    fragTextureId = seg.textureId;

    // Base UV: U along length [0..1], V across width [0..1]
    vec2 baseUV = vec2(tSeg, sideSign * 0.5 + 0.5);

    // Apply GPU-side panning over time along U coordinate
    float time = float(gameTime);
    vec2 pannedOffset = seg.uv.xy + vec2(seg.panSpeed * time, 0.0);

    outUV = baseUV * seg.uv.zw + pannedOffset;
}