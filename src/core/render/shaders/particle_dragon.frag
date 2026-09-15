#version 450

// Descriptor Set 0: Game UBO (time)
layout(set = 0, binding = 0) uniform Game {
    double gameTime;
} game;

// Descriptor Set 1: Scene UBO (camera, lights, matrices)
layout(set = 1, binding = 0) uniform Scene {
    mat4 viewProj;
    mat4 view;
    mat4 proj;
    vec4 cameraPos;
    vec4 sun;
} scene;

// Inputs from sphere.vert
layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragCenter;
layout(location = 2) in vec4 fragColor;
layout(location = 3) in float fragRadius;
layout(location = 4) in vec4 fragExtra;

layout(location = 0) out vec4 outColor;

void main() {
    // 1. Ray setup from camera position through quad fragment
    vec3 ro = scene.cameraPos.xyz;
    vec3 rd = normalize(fragWorldPos - ro);

    // 2. Analytical Ray-Sphere intersection
    vec3 oc = ro - fragCenter;
    float b = dot(oc, rd);
    float c = dot(oc, oc) - (fragRadius * fragRadius);
    float h = b * b - c;

    // Discard fragments outside the true 3D sphere volume
    if (h < 0.0) {
        discard;
    }

    // 3. Compute surface hit point and local sphere UVs
    float t = -b - sqrt(h);
    vec3 hitPos = ro + rd * t;
    vec3 normal = normalize(hitPos - fragCenter);
    
    // Map 3D surface normal to 2D [-1, 1] projection space for the particle math
    vec2 UVN = normal.xy;

    // 4. Particle calculation logic
    vec4 cf = vec4(0.0);
    float T = float(game.gameTime) * 2.0;
    float IP = 6000.0;
    float OR, HZ, DC, PO;

    for (; IP >= 0.0; IP -= 20.0 / 3.0) {
        OR = 5.0 * cos(IP / 44.0);
        HZ = IP / 506.0 - 15.0;
        DC = length(vec2(OR, HZ)) / 3.0;
        PO = DC / 2.0 - T / 3.0 + mod(IP, 2.0) * 3.0;

        float CX = (93.22 + DC * DC + OR * OR) * sin(PO) 
                 + DC * DC * DC / 4.0 * cos(T * 3.0 - DC * DC / 4.0);
        
        float CY = -99.0 * cos(PO / 2.0) 
                 - 4.0 * sin(OR + OR) 
                 - IP * OR * HZ / 19481.0 / sin(HZ / 2.0);

        vec2 PP = vec2(CX, CY) / 160.8;

        vec3 CP = 0.5 + 0.299 * cos(PO + T + vec3(0.0, 1.0, 2.0));
        float RG = length(UVN - PP) + 0.001;

        cf.rgb += CP * (0.000468 / RG);
    }

    cf = pow(cf, vec4(1.7));

    // 5. Fresnel edge mask for spherical depth perception
    vec3 viewDir = -rd;
    float NdotV = max(dot(normal, viewDir), 0.0);
    float fresnel = pow(1.0 - NdotV, 2.0);

    // Combine procedural glow with instance color (fragColor) and apply alpha falloff
    vec3 finalColor = cf.rgb * fragColor.rgb;
    float finalAlpha = clamp(fragColor.a * (NdotV + fresnel), 0.0, 1.0);

    // Write true 3D spherical depth buffer value
    vec4 clipPos = scene.viewProj * vec4(hitPos, 1.0);
    gl_FragDepth = clipPos.z / clipPos.w;

    outColor = vec4(finalColor, finalAlpha);
}