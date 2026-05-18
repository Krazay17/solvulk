#version 450

layout(set = 1, binding = 0) uniform sampler2D textures[64];

layout(location = 0) in vec3 viewDir;
layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;
const uint SKYBOX_TEX_ID = 5;  // SOL_TEXTURE_SKYBOX — match your enum

vec2 dirToEquirect(vec3 dir) {
    float u = atan(dir.z, dir.x) / (2.0 * PI) + 0.5;
    float v = asin(dir.y) / PI + 0.5;
    return vec2(u, v);
}

void main() {
    vec3 dir = normalize(viewDir);
    vec2 uv = dirToEquirect(dir);
    outColor = texture(textures[SKYBOX_TEX_ID], uv);
}