#version 450

struct Rect {
    vec4 pos;        // xy = world position, z = depth, w = scale
    vec4 color;
    vec4 dims;       // xy = full width/height, z = spin angle, w = fill (0..1)
    vec4 uv;         // xy = UV offset, zw = UV scale
    uint type;
    uint flags;
    uint textureID;
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

const uint UI_FLAG_FILL_VERTICAL = 1u << 1;
const uint UI_FLAG_FILL_INVERT   = 1u << 2;

const vec2 corners[6] = vec2[](
    vec2(0, 0), vec2(0, 1), vec2(1, 1),
    vec2(0, 0), vec2(1, 1), vec2(1, 0)
);

void main() {
    Rect r = quads[gl_InstanceIndex];
    vec2 v = corners[gl_VertexIndex];
    
    rectDims = r.dims.xy;   // full size for the fragment's hollow-border test
    
    // Pick which axis to fill: x by default, y if UI_FLAG_FILL_VERTICAL.
    vec2 fillAxis  = ((r.flags & UI_FLAG_FILL_VERTICAL) != 0u) ? vec2(0, 1) : vec2(1, 0);
    vec2 otherAxis = vec2(1.0) - fillAxis;
    float fill = r.dims.w;
    
    // Scale only the fill axis by fill; leave the other axis at full size.
    vec2 scaledDims = rectDims * (otherAxis + fillAxis * fill);
    
    // Anchor: by default v=(0,0) corner stays put. INVERT flips to the v=(1,1) corner.
    vec2 anchor = ((r.flags & UI_FLAG_FILL_INVERT) != 0u)
        ? (rectDims - scaledDims) * fillAxis
        : vec2(0.0);
    
    localPos = v * scaledDims + anchor;
    
    // Apply scale and rotation about the rect center.
    vec2 worldPos = localPos * r.pos.w;
    float ang = r.dims.z;
    if (ang != 0.0) {
        vec2 c = rectDims * r.pos.w * 0.5;
        vec2 d = worldPos - c;
        float ca = cos(ang), sa = sin(ang);
        worldPos = vec2(d.x * ca - d.y * sa, d.x * sa + d.y * ca) + c;
    }
    
    // UV scales the same axis as the fill — texture clips with the bar.
    vec2 uvScale = otherAxis + fillAxis * fill;
    fragUV = r.uv.xy + v * r.uv.zw * uvScale;
    
    fragColor = r.color;
    fragType = r.type;
    fragTextureId = r.textureID;
    fragFlags = r.flags;
    float correctedDepth = 1.0 - r.pos.z;
    gl_Position = ortho2d * vec4(worldPos + r.pos.xy, correctedDepth, 1.0);
}