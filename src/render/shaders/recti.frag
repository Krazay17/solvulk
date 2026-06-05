#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;
layout(location = 2) in vec2 localPos;
layout(location = 3) in vec2 rectDims;
layout(location = 4) flat in uint fragType;
layout(location = 5) flat in uint fragTextureId;
layout(location = 6) flat in uint fragFlags;

layout(set = 2, binding = 0) uniform sampler2D textures[64];

layout(location = 0) out vec4 outColor;

const uint UI_FLAG_HOLLOW = 1u << 0;

void main() {
    // Hollow border: discard the interior, keep the frame.
    if ((fragFlags & UI_FLAG_HOLLOW) != 0u) {
        float t = (fragType > 0u) ? float(fragType) : 4.0;
        if (localPos.x > t && localPos.x < rectDims.x - t &&
            localPos.y > t && localPos.y < rectDims.y - t)
            discard;
    }
    // Texture 0 reserved for the font atlas (handled by another shader).
    // Any rect that wants a texture sets textureID >= 1.
    vec4 tex = (fragTextureId != 0u)
        ? texture(textures[fragTextureId], fragUV)
        : vec4(1.0);
    
    outColor = tex * fragColor;
}