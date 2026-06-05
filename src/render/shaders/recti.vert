#version 450

struct Rect {
    vec4 pos;        // xy = world position, z = depth, w = scale
    vec4 color;
    vec4 dims;       // xy = width/height, z = spin angle, w = fill (UV.x scale)
    vec4 uv;         // xy = UV offset, zw = UV scale
    uint type;       // freeform: e.g. border thickness in pixels
    uint flags;      // bit 0 = hollow
    uint textureID;  // 0 = solid color (font atlas owns slot 0)
    uint _pad;
};

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec4 fragColor;
layout(location = 2) out vec2 localPos;
layout(location = 3) out vec2 rectDims;
layout(location = 4) flat out uint fragType;
layout(location = 5) flat out uint fragTextureId;
layout(location = 6) flat out uint fragFlags;

layout(set = 0, binding = 0) uniform Ortho { mat4 ortho2d; };
layout(set = 1, binding = 0) readonly buffer Rects { Rect quads[]; };

const vec2 corners[6] = vec2[](
    vec2(0, 0), vec2(0, 1), vec2(1, 1),
    vec2(0, 0), vec2(1, 1), vec2(1, 0)
);

void main() {
    Rect r = quads[gl_InstanceIndex];
    vec2 v = corners[gl_VertexIndex];
    
    // Local-space position for the fragment's hollow-border test (pre-rotation, pre-scale).
    rectDims = r.dims.xy;
    localPos = v * rectDims;
    
    // Apply scale, then rotation about the rect center.
    vec2 worldPos = localPos * r.pos.w;
    float ang = r.dims.z;
    if (ang != 0.0) {
        vec2 c = rectDims * r.pos.w * 0.5;
        vec2 d = worldPos - c;
        float ca = cos(ang), sa = sin(ang);
        worldPos = vec2(d.x * ca - d.y * sa, d.x * sa + d.y * ca) + c;
    }
    
    // UV: offset + per-corner step, with X scaled by fill (dims.w).
    // When fill = 1.0, this is a no-op. When fill < 1.0, the texture clips.
    fragUV = r.uv.xy + v * r.uv.zw * vec2(r.dims.w, 1.0);
    
    fragColor     = r.color;
    fragType      = r.type;
    fragTextureId = r.textureID;
    fragFlags     = r.flags;
    
    gl_Position = ortho2d * vec4(worldPos + r.pos.xy, r.pos.z, 1.0);
}