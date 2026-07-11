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
    vec4 rect;      // xy = offset, zw = dims
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
// flags
const uint UI_FLAG_FILL_VERTICAL = 1u << 1;
const uint UI_FLAG_FILL_INVERT   = 1u << 2;
// type
const uint QUADTYPE_FACECAM = 0u;
const uint QUADTYPE_QUAT    = 1u;

vec3 rotateByQuat(vec4 q, vec3 v) {
    return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}

void main() {
    Quad q = quads[gl_InstanceIndex];
    vec2 corner = CORNERS[gl_VertexIndex];
    float size = q.pos.w;

    // Base dimensions from rect.zw
    vec2 halfDim = vec2(size, size);
    halfDim.x = q.rect.z > 0.0 ? q.rect.z * size : size;
    halfDim.y = q.rect.w > 0.0 ? q.rect.w * size : size;
    
    vec2 viewOffset = q.rect.xy;
    vec2 localExtent = halfDim;
    vec2 pivotShift = vec2(0.0);
    
    // --- 3D FILL PATTERN MATH ---
    float fill = (q.extra.x > 0.0) ? clamp(q.extra.x, 0.0, 1.0) : 1.0;
    
    // Determine fill axes
    vec2 fillAxis  = ((q.flags & UI_FLAG_FILL_VERTICAL) != 0u) ? vec2(0.0, 1.0) : vec2(1.0, 0.0);
    vec2 otherAxis = vec2(1.0) - fillAxis;
    
    // Scale the geometry along the selected fill axis
    localExtent = halfDim * (otherAxis + fillAxis * fill);
    
    // Shift the center origin so the bar drains from side-to-side rather than shrinking into the center
    pivotShift = halfDim * fillAxis * (fill - 1.0);
    if ((q.flags & UI_FLAG_FILL_INVERT) != 0u) {
        pivotShift *= -1.0; // Drain from opposite direction
    }

    vec3 worldPos;
        
    if (q.type == QUADTYPE_FACECAM) {
        float angle = q.rot.x;
        float c = cos(angle);
        float s = sin(angle);
        
        // Spin the layout corner bounds locally
        vec2 spun = vec2(corner.x * localExtent.x * c - corner.y * localExtent.y * s,
                         corner.x * localExtent.x * s + corner.y * localExtent.y * c);

        vec3 right = vec3(scene.view[0][0], scene.view[1][0], scene.view[2][0]);
        vec3 up    = vec3(scene.view[0][1], scene.view[1][1], scene.view[2][1]);

        // Shift viewOffset by pivotShift to keep the anchored edge stable
        worldPos = q.pos.xyz + 
                   right * (viewOffset.x + pivotShift.x + spun.x) + 
                   up    * (viewOffset.y + pivotShift.y + spun.y);
    } else {
        // QUADTYPE_QUAT
        vec3 localPos = vec3(pivotShift.x + corner.x * localExtent.x, 
                             pivotShift.y + corner.y * localExtent.y, 0.0);
        worldPos = q.pos.xyz + rotateByQuat(q.rot, localPos);
    }
    
    // Clip the UVs exactly alongside the fill scale
    vec2 uvLocal = vec2(corner.x, -corner.y) * 0.5 + 0.5;   
    vec2 uvScale = otherAxis + fillAxis * fill;
    fragUV = q.uv.xy + uvLocal * q.uv.zw * uvScale;
    
    fragColor     = q.color;
    fragExtra     = q.extra;
    fragType      = q.type;
    fragTextureId = q.textureId;
    fragFlags     = q.flags;
    
    gl_Position = scene.viewProjection * vec4(worldPos, 1.0);
}