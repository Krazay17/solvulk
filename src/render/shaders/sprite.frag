#version 450
layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;
layout(location = 2) flat in uint textureId;

layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform sampler2D textures[64];


void main() {
    vec4 tex = texture(textures[textureId], fragUV);
    if (tex.a < 0.1) discard;
    outColor = tex * fragColor;
}