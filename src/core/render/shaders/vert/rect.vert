#version 450

struct Rect {
    vec4 pos;        // xy = screen position, z = depth, w = uniform scale
    vec4 rect;       // xy = local pivot offset, zw = dimensions (width, height)
    vec4 color;      // tint / base color
    vec4 uv;         // xy = UV offset, zw = UV scale
    vec4 extra;      // custom fragment parameters (x=border, y=radius, etc.)
    float spin;      // rotation angle in radians
    uint flags;      // UI flags / state
    uint textureId;  // texture slot index
    uint _pad;       // explicit std430 16-byte alignment (total: 96 bytes)
};

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec4 fragColor;
layout(location = 2) out vec2 localPos;
layout(location = 3) out vec2 rectDims;
layout(location = 4) out vec4 fragExtra;
layout(location = 5) flat out uint fragTextureId;
layout(location = 6) flat out uint fragFlags;

layout(set = 0, binding = 0) uniform Ortho { 
    mat4 ortho2d; 
};

layout(set = 1, binding = 0) readonly buffer Rects { 
    Rect rects[]; 
};

const vec2 CORNERS[6] = vec2[](
    vec2(0.0, 0.0), vec2(0.0, 1.0), vec2(1.0, 1.0),
    vec2(0.0, 0.0), vec2(1.0, 1.0), vec2(1.0, 0.0)
);

void main() {
    Rect r = rects[gl_InstanceIndex];
    vec2 corner = CORNERS[gl_VertexIndex];

    float scale = (r.pos.w != 0.0) ? r.pos.w : 1.0;
    vec2 dims   = r.rect.zw;
    vec2 is_zero  = vec2(equal(r.uv.zw, vec2(0.0)));
    vec2 uv_scale = mix(r.uv.zw, vec2(1.0), is_zero);

    // Unscaled local position relative to pivot (e.g. rect.xy = -dims * 0.5 for center)
    vec2 unscaledLocal = r.rect.xy + (corner * dims);

    // Rotate around local (0,0) pivot
    vec2 rotatedLocal = unscaledLocal;
    if (r.spin != 0.0) {
        float c = cos(r.spin), s = sin(r.spin);
        rotatedLocal = vec2(unscaledLocal.x * c - unscaledLocal.y * s, 
                            unscaledLocal.x * s + unscaledLocal.y * c);
    }

    vec2 worldPos = r.pos.xy + (rotatedLocal * scale);

    // Fragment Shader Outputs
    fragUV        = r.uv.xy + corner * uv_scale;
    fragColor     = r.color;
    localPos      = unscaledLocal; // For SDF calculations
    rectDims      = dims;
    fragExtra     = r.extra;
    fragTextureId = r.textureId;
    fragFlags     = r.flags;

    float depth   = 1.0 - r.pos.z;
    gl_Position   = ortho2d * vec4(worldPos, depth, 1.0);
}