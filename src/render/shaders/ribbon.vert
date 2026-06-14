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
    vec4 uv;
    uint textureId, _pad0, _pad1, _pad2;
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

    // Correct: work in aspect-corrected space, then convert perp back to NDC
    vec2 dir  = normalize(vec2((ndcB.x - ndcA.x) * aspect, ndcB.y - ndcA.y));
    vec2 perp = vec2(-dir.y, dir.x);
    perp.x   /= aspect;


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

    fragTextureId = seg.textureId;
    outColor = color;

//    vec2 rawUV = vec2(s.x * 0.5 + 0.5, s.y);
//    outUV = vec2(1.0 - rawUV.y, rawUV.x);

    // Reconstruct normalized raw quad UV space from vertex signatures:
    // s.x matches -1.0 or +1.0 for cross-section edges. s.y is 0.0 (Start) or 1.0 (End)
    //vec2 rawUV = vec2(s.x * 0.5 + 0.5, s.y);

    // Apply the panning translation vector (.xy offset + raw layout mapped by .zw tile weight)
    //outUV = (rawUV * seg.uv.zw) + seg.uv.xy;

    // Isolate the horizontal edge signature coordinate (cross section)
    // s.x maps to -1.0 or +1.0 -> converts to 0.0 or 1.0 mapping across the width
    float uCoord = s.x * 0.5 + 0.5;

    // Pick the correct accumulated distance milestone depending on whether this vertex belongs to A or B
    float vDistance = isB ? seg.uv.y : seg.uv.x;

    // Optional: Scale vDistance by a texture repeat factor if you want it to wrap tightly
    float textureTilingFactor = 1.0f; 
    float vCoord = vDistance * textureTilingFactor;

    // Apply the panning animations to the final output vector coordinates
    // Adds your frame-by-frame time delta increments seamlessly
    outUV = vec2(uCoord + seg.uv.z, vCoord + seg.uv.w);
}