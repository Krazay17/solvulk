#version 450

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec4 outColor;
layout(location = 2) out vec4 fragExtra;
layout(location = 3) flat out uint fragType;
layout(location = 4) flat out uint fragTextureId;
layout(location = 5) flat out uint fragFlags;

struct RibbonSeg {
    vec4 posA; // .xyz = world pos, .w = half-width
    vec4 posB; // .xyz = world pos, .w = half-width
    vec4 colorA;
    vec4 colorB;
};

layout(set = 0, binding = 0) uniform Scene {
    mat4 viewProj;
    mat4 view;
    mat4 proj;
    vec4 cameraPos;
    vec4 sun;
    float aspect;
};

layout(set = 1, binding = 0) readonly buffer RibbonSSBO {
    RibbonSeg segs[];
};


void main()
{
    RibbonSeg seg = segs[gl_InstanceIndex];

    // Project both endpoints
    vec4 clipA = viewProj * vec4(seg.posA.xyz, 1.0);
    vec4 clipB = viewProj * vec4(seg.posB.xyz, 1.0);

    vec2 ndcA = clipA.xy / clipA.w;
    vec2 ndcB = clipB.xy / clipB.w;

    vec2 dir  = normalize(ndcB - ndcA);
    vec2 perp = vec2(-dir.y, dir.x);
    perp.x   /= aspect; // correct for aspect ratio

    // 6 verts = 2 triangles, quad corners:
    // vtx 0,3 = A-perp   vtx 1 = A+perp
    // vtx 2,4 = B-perp   vtx 5 = B+perp
    const vec2 signs[6] = vec2[](
        vec2(-1, 0), vec2( 1, 0), vec2(-1, 1),
        vec2( 1, 0), vec2( 1, 1), vec2(-1, 1)
    );
    vec2  s      = signs[gl_VertexIndex];
    bool  isB    = s.y > 0.5;
    vec4  base   = isB ? clipB : clipA;
    float halfW  = isB ? seg.posB.w : seg.posA.w;
    vec4  color  = isB ? seg.colorB : seg.colorA;

    // Remove * base.w to allow natural hardware perspective division!
    base.xy += perp * s.x * halfW; 
    gl_Position = base;

    fragTextureId = 13;
    outColor = color;

    vec2 rawUV = vec2(s.x * 0.5 + 0.5, s.y);
    outUV = vec2(1.0 - rawUV.y, rawUV.x);
}