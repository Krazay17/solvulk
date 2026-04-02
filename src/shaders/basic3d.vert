#version 450

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

struct ModelData {
    mat4 modelMatrix;
    vec4 color;
};

layout(std430, set = 0, binding = 0) readonly buffer ModelBuffer {
    ModelData instances[];
};

layout(push_constant) uniform Scene {
    mat4 viewProj;
} scene;

void main() {
    mat4 model = instances[gl_InstanceIndex].modelMatrix;
    gl_Position = scene.viewProj * model * vec4(inPos, 1.0);
    fragColor = vec4(inNormal, 1.0);
}