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
    vec4 sun; // xyz = direction, w = intensity
} scene;

layout(set = 4, binding = 0) uniform sampler2D textures[128];

layout(push_constant) uniform MeshMaterial {
    vec4 baseColor;
    vec4 emissive;

    vec2 textureScale;
    vec2 fogTextureScale;

    float metallic;
    float roughness;
    
    uint textureId;
    uint emissiveTextureId;
    uint normalTextureId;
    uint fogTextureId;

    uint _pad[2];
} material;

const uint FLAG_FRESNEL = 1u << 0;
const uint FLAG_YELLOW  = 1u << 1;
const uint FLAG_DAMAGED = 1u << 2;

const float PI = 3.14159265359;
const uint  SKY_TEXTURE_ID = 5u;

float SpecularGGX_Fast(vec3 N, vec3 H, float roughness)
{
    float a     = max(roughness * roughness, 0.01);
    float a2    = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float d     = (NdotH * NdotH) * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d + 0.0001);
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 PerturbNormal(vec3 N, vec3 V, vec2 uv, uint normalTexId)
{
    vec3 mapNormal = texture(textures[normalTexId], uv).xyz * 2.0 - 1.0;
    vec3 q1  = dFdx(fragWorldPos);
    vec3 q2  = dFdy(fragWorldPos);
    vec2 st1 = dFdx(uv);
    vec2 st2 = dFdy(uv);
    vec3 T   = normalize(q1 * st2.t - q2 * st1.t);
    vec3 B   = normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);
    return normalize(TBN * mapNormal);
}

vec3 TonemapPunchy(vec3 x)
{
    x *= 1.2;
    vec3 toe = x * x / (x + 0.08);
    return toe / (toe + 0.8);
}

void main()
{
    float fTime = float(time);

    // --- Albedo & alpha ---
    vec2 scaledUV = fragUV * material.textureScale;
    vec4 baseTex  = (material.textureId != 0u) ? texture(textures[material.textureId], scaledUV) : vec4(1.0);

    vec3 albedo = (fragColor.a > 0.0) ? mix(material.baseColor.rgb, fragColor.rgb, fragColor.a) : material.baseColor.rgb;
    albedo *= baseTex.rgb;

    float rawAlpha = (fragColor.a > 0.0) ? fragColor.a : material.baseColor.a;
    if (rawAlpha <= 0.0) {
        rawAlpha = 1.0;
    }

    float texAlpha = (material.textureId != 0u) ? baseTex.a : 1.0;
    if (texAlpha <= 0.0) {
        texAlpha = 1.0;
    }

    float alpha = rawAlpha * texAlpha;

    vec3 hybridFogEmmisive = vec3(0.0);
    if (material.fogTextureId != 0u)
    {
        vec2 fogUV     = fragUV * material.fogTextureScale + vec2(1, fTime * 0.05);
        vec4 fogSample = texture(textures[material.fogTextureId], fogUV);

        float invAlpha = 1.0 - baseTex.a;
        float glowMask = invAlpha * fogSample.a;

        albedo = mix(albedo, fogSample.rgb, glowMask);
        hybridFogEmmisive = fogSample.rgb * glowMask * 1.0;

        alpha = max(alpha, glowMask);
        float pulse = 3.75 + 0.25 * sin(fTime * 0.2);
        alpha *= pulse;
    }

    // --- Vectors ---
    vec3 V = normalize(scene.cameraPos.xyz - fragWorldPos);
    
    // Normal map is plugged back in safely here
    vec3 N = (material.normalTextureId != 0u)
                 ? PerturbNormal(normalize(fragNormal), V, scaledUV, material.normalTextureId)
                 : normalize(fragNormal);
                 
    vec3 L = normalize(scene.sun.xyz);
    vec3 H = normalize(V + L);

    float NdotV = max(dot(N, V), 0.0001);
    float NdotL = max(dot(N, L), 0.0);

    // --- Reflectivity ---
    vec3 F0 = mix(vec3(0.04), albedo, material.metallic);

    // --- Ambient Lighting Calculation ---
    vec3 ambientSpecular = vec3(0.0);
    vec3 ambientFill = vec3(0.02, 0.025, 0.035);
    if (material.metallic > 0.01)
    {
        vec3 R = reflect(-V, N);
        vec2 skyUV = vec2(atan(R.z, R.x) / (2.0 * PI) + 0.5, acos(clamp(R.y, -1.0, 1.0)) / PI);
        float horizonGlowMask = clamp(N.y + 0.5, 0.0, 1.0); 
        ambientSpecular = texture(textures[SKY_TEXTURE_ID], skyUV).rgb * F0 * horizonGlowMask;
    }
    vec3 ambientDiffuse = ambientFill * albedo * mix(0.2, 1.0, NdotL);
    vec3 ambient = mix(ambientDiffuse, ambientSpecular, material.metallic);

    // --- Direct Light Assembly (Cleaned up and protected) ---
    vec3 directLighting = vec3(0.0);
    if (NdotL > 0.0) 
    {
        float D = SpecularGGX_Fast(N, H, material.roughness);
        vec3  F = FresnelSchlick(max(dot(H, V), 0.0), F0);
        vec3  specular = vec3(D) * F / max(4.0 * NdotV * NdotL, 0.25);

        vec3 kD = (1.0 - material.metallic) * albedo;
        float sunIntensity = (scene.sun.w > 0.0) ? scene.sun.w : 1.0;
        
        directLighting = (kD / PI + specular) * sunIntensity * NdotL;
    }

    vec3 finalRGB = ambient + directLighting;

    // --- Emission ---
    vec3 emissive = material.emissive.rgb * material.emissive.a * 0.5;
    if (material.emissiveTextureId != 0u)
        emissive *= texture(textures[material.emissiveTextureId], fragUV).rgb;

    emissive += fragColor.rgb;
    finalRGB += emissive;
    finalRGB += hybridFogEmmisive;

    // --- Visual Flags ---
    if ((flags & FLAG_FRESNEL) != 0u)
    {
        float rim = pow(1.0 - NdotV, 3.0);
        finalRGB += rim * vec3(1.0);
    }
    if ((flags & FLAG_YELLOW) != 0u)
    {
        finalRGB += vec3(1.0, 1.0, 0.0);
    }

    float timeSinceHit = fTime - fragHitTime;
    if (timeSinceHit >= 0.0 && timeSinceHit < 0.3)
    {
        float flashAlpha = 1.0 - (timeSinceHit / 0.3);
        finalRGB = mix(finalRGB, vec3(1.0), flashAlpha * 0.85);
    }

    // --- Tonemap & Gamma ---
    finalRGB = TonemapPunchy(finalRGB);
    finalRGB = pow(finalRGB, vec3(1.0 / 2.2));

    outColor = vec4(finalRGB, alpha);
}