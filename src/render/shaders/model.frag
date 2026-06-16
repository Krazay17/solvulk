#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) flat in int instanceIndex;
layout(location = 4) flat in uint flags;
layout(location = 5) flat in float fragHitTime;
layout(location = 6) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform Util {
    double time;
};
layout(set = 1, binding = 0) uniform Scene {
    mat4 viewProj;
    mat4 view;
    mat4 proj;
    vec4 cameraPos;
    vec4 sun;
} scene;
layout(set=4,binding=0)uniform sampler2D textures[64];

layout(push_constant) uniform MeshMaterial {
    vec4 baseColor;
    vec4 emissive;
    float metallic;
    float roughness;
    uint textureId;
} material;

const uint FLAG_FRESNEL = 1u << 0;
const uint FLAG_YELLOW = 1u << 1;
const uint FLAG_DAMAGED = 1u << 2;

void main() {
    vec3 N = normalize(fragNormal);
    vec3 lightDir = normalize(scene.sun.xyz);
    float NdotL = max(dot(N, lightDir), 0.0);

    vec4 tex = (material.textureId != 0u)
        ? texture(textures[material.textureId], fragUV)
        : vec4(1.0);

    vec3 color = fragColor.a > 0.0 ? fragColor.rgb + material.baseColor.rgb : material.baseColor.rgb;
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

    vec3 emissive_color = material.emissive.rgb;
    float emissive_strength = material.emissive.a;
    vec3 emissive = emissive_color * emissive_strength;

    vec3 lighting = ambient + diffuse + specular + emissive;

    vec4 finalRGB = vec4(lighting * tex.rgb, alpha);

    if ((flags & FLAG_FRESNEL) != 0u) {
            float fresnelTerm = 1.0 - max(dot(N, viewDir), 0.0);
            finalRGB.rgb += pow(fresnelTerm, 4.0) * vec3(1.0);}
    if ((flags & FLAG_YELLOW) != 0u) {
            finalRGB.rgb += vec3(1.0,1.0,0);}

    // 1. Calculate how long ago the hit happened
    float timeSinceHit = float(time) - fragHitTime;

    // 2. If it's within our 0.3 second window, apply a fading white flash
    if (timeSinceHit >= 0.0 && timeSinceHit < 0.3) {
        // Linear fade out over the 0.3 seconds
        float flashAlpha = 1.0 - (timeSinceHit / 0.3); 
        // Pure white flash mix
        finalRGB.rgb = mix(finalRGB.rgb, vec3(1.0, 1.0, 1.0), flashAlpha * 0.8);}

    outColor = finalRGB;
}