#version 450
layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;
layout(location = 2) in vec4 fragExtra;
layout(location = 3) flat in uint fragTextureId;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform Game {
    double gameTime;
} game;

layout(set = 1, binding = 0) uniform Scene {
    mat4 viewProjection;
    mat4 view;
    mat4 proj;
    vec4 cameraPos;
    vec4 sun;
} scene;

layout(set = 3, binding = 0) uniform sampler2D textures[64];


void main() {
    vec4 tex = texture(textures[fragTextureId], fragUV);
    if (tex.a < 0.1) discard;
    outColor = tex * fragColor;
}