#version 450
layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;
layout(location = 2) in vec4 fragExtra;
layout(location = 3) flat in uint fragType;
layout(location = 4) flat in uint fragTextureId;
layout(location = 5) flat in uint fragFlags;

layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform sampler2D textures[64];


void main() {
    vec4 tex = texture(textures[fragTextureId], fragUV);
    if (tex.a < 0.1) discard;
    outColor = tex * fragColor;
}