#version 450

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragMaterial;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

struct ModelData {
    mat4 modelMatrix;
    vec4 color;
    vec4 material;
};

layout(std430, set = 0, binding = 0) readonly buffer ModelBuffer {
    ModelData instances[];
};

layout(set=0, binding=0) uniform Scene {
    mat4 viewProj;
} scene;

void main() {
    ModelData inst = instances[gl_InstanceIndex];
    mat4 model = inst.modelMatrix;
    gl_Position = scene.viewProj * model * vec4(inPos, 1.0);
    fragColor = inst.color;
    fragNormal = mat3(model) * inNormal;
    fragMaterial = inst.material.xy;
}