#version 450

layout(set = 0, binding = 0) uniform Game {
    double t;
};

layout(set = 1, binding = 0) uniform Scene {
    mat4 viewProj;
    mat4 view;
    mat4 proj;
    vec4 cameraPos;
    vec4 sun; // sun.w can pass global time
} scene;

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragCenter;
layout(location = 2) in vec4 fragColor;
layout(location = 3) in float fragRadius;
layout(location = 4) in vec4 fragExtra; // fragExtra.x = animation time

layout(location = 0) out vec4 outColor;

// --- PROCEDURAL 3D DOMAIN-WARPED ENERGY FIELD ---
float EnergyField(vec3 p, float time) {
    vec3 q = p;

    // First domain warp: swirl coordinates using trigonometric vectors
    q += sin(p.yzx * 3.0 + vec3(time * 1.5, time * 1.2, time * 1.8)) * 0.35;
    
    // Second domain warp: add finer turbulent rotation
    q += cos(q.zxy * 6.0 - vec3(time * 2.0, time * 1.4, time * 2.2)) * 0.18;
    
    // Evaluate intersecting spherical wave patterns
    float f1 = sin(q.x * 8.0 + q.y * 8.0 + q.z * 8.0);
    float f2 = cos(q.x * 14.0 - q.y * 12.0 + q.z * 10.0 + time * 3.0);
    
    // Turn zero-crossings into sharp energy arcs/filaments
    float plasma = abs(f1 * f2);
    return pow(0.025 / (plasma + 0.025), 1.3);
}

// ACES Tone Mapping for filmic HDR bright-glow handling
vec3 ToneMap_ACES(vec3 x) {
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    float time = float(t);
    // 1. Ray setup through the sphere billboard quad
    vec3 ro = scene.cameraPos.xyz;
    vec3 rd = normalize(fragWorldPos - ro);

    // 2. Ray-Sphere intersection
    vec3 oc = ro - fragCenter;
    float b = dot(oc, rd);
    float c = dot(oc, oc) - (fragRadius * fragRadius);
    float h = b * b - c;

    if (h < 0.0) {
        discard;
    }

    float t = -b - sqrt(h);
    vec3 hitPos = ro + rd * t;
    vec3 normal = normalize(hitPos - fragCenter);
    vec3 viewDir = -rd;

    // Local 3D unit coordinates [-1, 1] relative to sphere origin
    vec3 localPos = (hitPos - fragCenter) / fragRadius;

    // 3. Fresnel edge glow calculations
    float NdotV = max(dot(normal, viewDir), 0.0);
    float fresnelPower = (fragExtra.y > 0.0) ? fragExtra.y : 3.0;
    float fresnel = pow(1.0 - NdotV, fresnelPower);

    // 4. Sample procedural energy arcs & core pulse
    float energyArcs = EnergyField(localPos, time);
    float corePulse  = sin(time * 3.0 - length(localPos) * 6.0) * 0.5 + 0.5;

    // 5. Build high-dynamic-range color profile
    vec3 baseTint   = fragColor.rgb;
    vec3 hotCore    = mix(baseTint, vec3(1.0), 0.75); // Hot white-ish core for high energy
    vec3 rimGlow    = baseTint * 3.5;

    vec3 hdrColor = vec3(0.0);
    hdrColor += hotCore * energyArcs * 1.2;          // Electric filaments
    hdrColor += rimGlow * fresnel * 2.5;             // Intense glowing shield rim
    hdrColor += baseTint * corePulse * 0.35;         // Internal glowing core

    // Alpha blending: transparent center, dense rim & energy arcs
    float alpha = clamp(fresnel * 0.85 + energyArcs * 0.35 + 0.1, 0.0, 1.0) * fragColor.a;

    // 6. Tone mapping & sRGB gamma output
    vec3 ldrColor   = ToneMap_ACES(hdrColor);
    vec3 finalColor = pow(ldrColor, vec3(1.0 / 2.2));

    // 7. Write exact 3D sphere depth
    vec4 clipPos = scene.viewProj * vec4(hitPos, 1.0);
    gl_FragDepth = clipPos.z / clipPos.w;

    outColor = vec4(finalColor, alpha);
}