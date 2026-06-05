#version 450

struct Glyph {
    vec4 pos;    // xy = top-left in screen space, zw = width/height
    vec4 color;
    vec4 uv;     // xy = UV offset, zw = UV scale
};

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec4 fragColor;

layout(set = 0, binding = 0) uniform Ortho { mat4 ortho2d; };
layout(set = 1, binding = 0) readonly buffer Glyphs { Glyph glyphs[]; };

const vec2 corners[6] = vec2[](
    vec2(0, 0), vec2(0, 1), vec2(1, 1),
    vec2(0, 0), vec2(1, 1), vec2(1, 0)
);

void main() {
    Glyph g = glyphs[gl_InstanceIndex];
    vec2 v = corners[gl_VertexIndex];
    
    // pos.xy = origin, pos.zw = dimensions → corner-scaled local rect
    vec2 screenPos = g.pos.xy + v * g.pos.zw;
    
    // uv.xy = atlas offset, uv.zw = atlas region size
    fragUV = g.uv.xy + v * g.uv.zw;
    
    fragColor = g.color;
    
    // z=0 for 2D — avoids the depth-clip issue you hit with RectI
    gl_Position = ortho2d * vec4(screenPos, 0.0, 1.0);
}