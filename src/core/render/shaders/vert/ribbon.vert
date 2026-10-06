#version 450

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec4 outColor;
layout(location = 3) flat out uint fragTextureId;

struct RibbonSeg {
    vec4 posA;
    vec4 posB;
    vec4 colorA;
    vec4 colorB;
    vec4 uv;
    vec4 dirA;
    vec4 dirB;
    uint textureId;
    uint flags;
    float panSpeed;
    float spin;
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

vec3 MakeRibbonSide(vec3 tangent, float spin)
{
    vec3 refUp = vec3(0.0, 1.0, 0.0);

    // Avoid degeneracy when tangent is parallel to world up.
    if (abs(dot(tangent, refUp)) > 0.99)
        refUp = vec3(1.0, 0.0, 0.0);

    // Base width direction perpendicular to the ribbon.
    vec3 side = normalize(cross(tangent, refUp));

    // Rotate the width direction around the ribbon tangent.
    float s = sin(spin);
    float c = cos(spin);

    return side * c + cross(tangent, side) * s;
}

const vec2 CORNERS[6] = vec2[](
    vec2(-1.0, 0.0), // Left A
    vec2( 1.0, 0.0), // Right A
    vec2(-1.0, 1.0), // Left B

    vec2( 1.0, 0.0), // Right A
    vec2( 1.0, 1.0), // Right B
    vec2(-1.0, 1.0)  // Left B
);

void main()
{
    RibbonSeg seg = segs[gl_InstanceIndex];
    vec2 corner   = CORNERS[gl_VertexIndex];

    float sideSign = corner.x;
    float tSeg     = corner.y;

    vec3 basePos = mix(seg.posA.xyz, seg.posB.xyz, tSeg);
    float halfW  = mix(seg.posA.w, seg.posB.w, tSeg);

    // Use the shared tangent at the appropriate endpoint.
    vec3 segDir = normalize(
        (tSeg < 0.5) ? seg.dirA.xyz : seg.dirB.xyz
    );

    bool alignWorldUp = (seg.flags & 1u) != 0u;

    vec3 sideDir;

    if (alignWorldUp)
    {
        sideDir = MakeRibbonSide(segDir, seg.spin);
    }
    else
    {
        vec3 viewDir = normalize(cameraPos.xyz - basePos);

        sideDir = cross(segDir, viewDir);

        float lenSq = dot(sideDir, sideDir);

        sideDir = (lenSq > 0.0001)
            ? normalize(sideDir)
            : vec3(1.0, 0.0, 0.0);
    }

    vec3 worldPos = basePos + sideDir * (sideSign * halfW);

    gl_Position = viewProj * vec4(worldPos, 1.0);

    outColor      = mix(seg.colorA, seg.colorB, tSeg);
    fragTextureId = seg.textureId;

    vec2 baseUV = vec2(
        tSeg,
        sideSign * 0.5 + 0.5
    );

    float time = float(gameTime);

    vec2 pannedOffset =
        seg.uv.zw +
        vec2(seg.panSpeed * time, 0.0);

    outUV = baseUV * seg.uv.xy + pannedOffset;
}