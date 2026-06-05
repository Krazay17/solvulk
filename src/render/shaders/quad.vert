#version 450

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec4 fragColor;
layout(location = 2) out vec4 fragExtra;
layout(location = 3) flat out uint fragType;
layout(location = 4) flat out uint fragTextureId;
layout(location = 5) flat out uint fragFlags;

layout(set = 0, binding = 0) uniform Scene {
    mat4 viewProjection;
    mat4 view;
    mat4 proj;
    vec4 cameraPos;
    vec4 sun;
} scene;

struct Quad {
    vec4 pos;       // xyz = world position, w = size
    vec4 rot;       // quaternion OR rot.x = spin angle
    vec4 color;
    vec4 uv;
    vec4 extra;
    uint type;      // QUADTYPE_FACECAM (0) or QUADTYPE_QUAT (1)
    uint flags;
    uint textureId;
    uint _pad;
};

layout(set = 1, binding = 0) readonly buffer Quads {
    Quad quads[];
};

const vec2 CORNERS[6] = vec2[](
    vec2(-1, -1), vec2( 1, -1), vec2( 1,  1),
    vec2(-1, -1), vec2( 1,  1), vec2(-1,  1)
);

// Explicitly mapping to your QuadType Enum
const uint QUADTYPE_FACECAM = 0u;
const uint QUADTYPE_QUAT    = 1u;

vec3 rotateByQuat(vec4 q, vec3 v) {
    return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}

void main() {
    Quad q = quads[gl_InstanceIndex];
    vec2 corner = CORNERS[gl_VertexIndex];
    float size = q.pos.w;
    vec3 worldPos;
        
    if (q.type == QUADTYPE_FACECAM) {
        float angle = q.rot.x;
        float c = cos(angle);
        float s = sin(angle);
        vec2 spun = vec2(corner.x * c - corner.y * s,
                         corner.x * s + corner.y * c);

        vec3 right = vec3(scene.view[0][0], scene.view[1][0], scene.view[2][0]);
        vec3 up    = vec3(scene.view[0][1], scene.view[1][1], scene.view[2][1]);

        // Non-uniform half-dimensions from extra.zw, else uniform from pos.w
        vec2 halfDim = (q.extra.z > 0.0) ? q.extra.zw : vec2(size, size);
        worldPos = q.pos.xyz + right * spun.x * halfDim.x + up * spun.y * halfDim.y;
    } else {
        // QUADTYPE_QUAT
        vec2 halfDim = (q.extra.z > 0.0) ? q.extra.zw : vec2(size, size);
        vec3 localPos = vec3(corner.x * halfDim.x, corner.y * halfDim.y, 0.0);
        worldPos = q.pos.xyz + rotateByQuat(q.rot, localPos);
    }
    
    vec2 uvLocal = vec2(corner.x, -corner.y) * 0.5 + 0.5;   
    fragUV = q.uv.xy + uvLocal * q.uv.zw;
    
    fragColor     = q.color;
    fragExtra     = q.extra;    
    fragType      = q.type;
    fragTextureId = q.textureId;
    fragFlags     = q.flags;
    
    gl_Position = scene.viewProjection * vec4(worldPos, 1.0);
}