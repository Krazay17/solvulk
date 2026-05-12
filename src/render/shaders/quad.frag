#version 450

layout(set = 2, binding = 0) uniform sampler2D spriteAtlas;

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 tex = texture(spriteAtlas, fragUV);
    outColor = vec4(tex.rgb * tex.a, tex.a) * fragColor;
}