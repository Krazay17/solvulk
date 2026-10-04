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
    vec4 dirA;       // .xyz = Shared tangent at A
    vec4 dirB;       // .xyz = Shared tangent at B
    uint textureId;  
    uint flags;      
    float panSpeed;  
    uint _pad;       
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

    float sideSign = corner.x; // -1.0 or +1.0 (Width)
    float tSeg     = corner.y; //  0.0 or  1.0 (Length)

    vec3  basePos = mix(seg.posA.xyz, seg.posB.xyz, tSeg);
    float halfW   = mix(seg.posA.w, seg.posB.w, tSeg);

    // Pick the shared tangent for the specific vertex being drawn
    vec3 segDir = (tSeg < 0.5) ? seg.dirA.xyz : seg.dirB.xyz;

    // The rest of the billboarding logic is exactly the same!
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

    vec3 worldPos = basePos + sideDir * (sideSign * halfW);
    gl_Position   = viewProj * vec4(worldPos, 1.0);

    // Color & Texture ID
    outColor      = mix(seg.colorA, seg.colorB, tSeg);
    fragTextureId = seg.textureId;

    // tSeg maps to U (length), sideSign maps to V (width)
    vec2 baseUV = vec2(tSeg, sideSign * 0.5 + 0.5);

    float time = float(gameTime);
    
    // Pans along the U axis (length)
    vec2 pannedOffset = seg.uv.zw + vec2(seg.panSpeed * time, 0.0);

    outUV = baseUV * seg.uv.xy + pannedOffset;
}