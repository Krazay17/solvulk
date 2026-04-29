#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 4) flat in int inInstanceIndex;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform Scene {
    mat4 viewProj;
    mat4 view;
    mat4 proj;
    vec4 cameraPos;
    vec4 sun;
} scene;

struct Flags{
    uint flags;
};
layout(std430, set = 2, binding = 0) readonly buffer FlagsBuffer {
    Flags instances[];
};

layout(push_constant) uniform MeshMaterial {
    vec4 baseColor;
    float metallic;
    float roughness;
} material;

const uint FLAG_FRESNEL = 1u << 0;

void main() {
    vec3 lightDir = normalize(scene.sun.xyz);
    float NdotL = max(dot(normalize(fragNormal), lightDir), 0.0);

    vec3 color = fragColor.a > 0.0 ? fragColor.rgb : material.baseColor.rgb;
    float alpha = fragColor.a > 0.0 ? fragColor.a : material.baseColor.a;

    // Metals reflect the base color, dielectrics reflect white
    vec3 diffuseColor = color * (1.0 - material.metallic);
    vec3 ambient = diffuseColor * 0.15;
    vec3 diffuse = diffuseColor * NdotL;

    // Simple specular — rougher surfaces have wider, dimmer highlights
    vec3 viewDir = normalize(scene.cameraPos.xyz - fragWorldPos);
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normalize(fragNormal), halfDir), 0.0),
                     mix(256.0, 4.0, material.roughness));
    vec3 specColor = mix(vec3(0.04), color, material.metallic);
    vec3 specular = specColor * spec * (1.0 - material.roughness * 0.5);

    vec4 finalRGB = vec4(ambient + diffuse + specular, alpha);
    if ((instances[inInstanceIndex].flags & FLAG_FRESNEL) != 0u) {
            float fresnelTerm = 1.0 - max(dot(normalize(fragNormal), viewDir), 0.0);
            // Using pow(x, 4.0) for a standard rim light effect
            finalRGB.rgb += pow(fresnelTerm, 4.0) * vec3(1.0);
        }

    outColor = finalRGB;
}