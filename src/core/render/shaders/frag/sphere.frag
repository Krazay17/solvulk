#version 450

layout(set = 1, binding = 0) uniform Scene {
    mat4 viewProj;
    mat4 view;
    mat4 proj;
    vec4 cameraPos;
    vec4 sun;
} scene;

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragCenter;
layout(location = 2) in vec4 fragColor;
layout(location = 3) in float fragRadius;
layout(location = 4) in vec4 fragExtra;

layout(location = 0) out vec4 outColor;

void main() {
    // Ray setup from camera position through quad fragment
    vec3 ro = scene.cameraPos.xyz;
    vec3 rd = normalize(fragWorldPos - ro);

    // Ray-Sphere intersection math
    vec3 oc = ro - fragCenter;
    float b = dot(oc, rd);
    float c = dot(oc, oc) - (fragRadius * fragRadius);
    float h = b * b - c;

    // Discard pixels outside the sphere radius
    if (h < 0.0) {
        discard;
    }

    // Hit distance and surface normal reconstruction
    float t = -b - sqrt(h);
    vec3 hitPos = ro + rd * t;
    vec3 normal = normalize(hitPos - fragCenter);
    vec3 viewDir = -rd;

    // Fresnel factor (grazing angle intensity)
    float NdotV = max(dot(normal, viewDir), 0.0);
    float fresnelPower = (fragExtra.x > 0.0) ? fragExtra.x : 3.0;
    float fresnel = pow(1.0 - NdotV, fresnelPower);

    // Simple ambient + sun light shading
    vec3 lightDir = (length(scene.sun.xyz) > 0.01) ? normalize(scene.sun.xyz) : vec3(0.0, 1.0, 0.0);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 ambient = vec3(0.2);
    vec3 baseDiffuse = fragColor.rgb * (diff + ambient);

    // Blend base color with fresnel rim intensity
    vec3 finalColor = mix(baseDiffuse, fragColor.rgb, fresnel) + (fragColor.rgb * fresnel);
    float finalAlpha = clamp(fragColor.a * (NdotV + fresnel), 0.0, 1.0);

    // Write true 3D spherical depth instead of flat billboard quad depth
    vec4 clipPos = scene.viewProj * vec4(hitPos, 1.0);
    gl_FragDepth = clipPos.z / clipPos.w;

    outColor = vec4(finalColor, finalAlpha);
}