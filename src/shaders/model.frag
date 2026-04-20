#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform Scene {
    mat4 viewProj;
    mat4 view;
    mat4 proj;
    vec4 cameraPos;
} scene;

layout(push_constant) uniform MeshMaterial {
    vec4 baseColor;
    float metallic;
    float roughness;
} material;

void main() {
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
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

    outColor = vec4(ambient + diffuse + specular, alpha);
}