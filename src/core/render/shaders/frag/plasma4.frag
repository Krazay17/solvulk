#version 450

layout(set = 0, binding = 0) uniform Game {
    double time;
};

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

// ─── Hash / Noise helpers ───────────────────────────────────────────────────
float hash11(float p) {
    p = fract(p * 0.1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}

float hash21(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float hash31(vec3 p) {
    p  = fract(p * 0.1031);
    p += dot(p, p.yzx + 33.33);
    return fract((p.x + p.y) * p.z);
}

// Value noise
float vnoise(vec3 x) {
    vec3 i = floor(x);
    vec3 f = fract(x);
    f = f * f * (3.0 - 2.0 * f);

    float n = i.x + i.y * 157.0 + 113.0 * i.z;
    return mix(mix(mix(hash11(n +   0.0), hash11(n +   1.0), f.x),
                   mix(hash11(n + 157.0), hash11(n + 158.0), f.x), f.y),
               mix(mix(hash11(n + 113.0), hash11(n + 114.0), f.x),
                   mix(hash11(n + 270.0), hash11(n + 271.0), f.x), f.y), f.z);
}

// Fractional Brownian Motion
float fbm(vec3 p, int octaves) {
    float v = 0.0;
    float a = 0.5;
    vec3 shift = vec3(100.0);
    for (int i = 0; i < octaves; ++i) {
        v += a * vnoise(p);
        p = p * 2.1 + shift;
        a *= 0.5;
    }
    return v;
}

// Domain-warped chaos
float plasmaField(vec3 p, float t) {
    // Heavy domain warp
    vec3 q = p;
    q += 0.35 * sin(q.yzx * 3.7 + t * 1.7);
    q += 0.25 * cos(q.zxy * 5.3 - t * 2.3);
    q += 0.15 * sin(q.xzy * 8.1 + t * 3.1);

    float n1 = fbm(q * 1.8 + t * 0.4, 5);
    float n2 = fbm(q * 3.4 - t * 0.6 + 17.0, 4);
    float n3 = fbm(q * 6.2 + t * 0.9 + 31.0, 3);

    // Chaotic combination
    float field = n1 * 0.55 + n2 * 0.30 + n3 * 0.15;
    field = field * field * (3.0 - 2.0 * field);          // contrast
    field = mix(field, abs(field * 2.0 - 1.0), 0.35);     // more turbulence
    return field;
}

// ─── Main ───────────────────────────────────────────────────────────────────
void main() {
    float t = float(time);

    vec3 ro = scene.cameraPos.xyz;
    vec3 rd = normalize(fragWorldPos - ro);

    // Ray-sphere intersection
    vec3 oc = ro - fragCenter;
    float b = dot(oc, rd);
    float c = dot(oc, oc) - fragRadius * fragRadius;
    float h = b * b - c;

    if (h < 0.0) discard;

    h = sqrt(h);
    float tNear = -b - h;
    float tFar  = -b + h;

    // Prefer front hit; fall back to back hit if camera is inside
    float hitT = (tNear > 0.0) ? tNear : tFar;
    if (hitT < 0.0) discard;

    vec3 hitPos = ro + rd * hitT;
    vec3 normal = normalize(hitPos - fragCenter);

    // Normalized radial coordinate (0 = center, 1 = surface)
    float r = length(hitPos - fragCenter) / fragRadius;
    float viewDot = max(dot(normal, -rd), 0.0);

    // ── Plasma field ────────────────────────────────────────────────────────
    // Scale + animate in world space, then slightly bias by radius
    vec3 noiseCoord = (hitPos - fragCenter) * (2.8 / fragRadius);
    noiseCoord += fragExtra.xyz * 0.15;          // optional per-sphere variation

    float field = plasmaField(noiseCoord, t * 1.15);

    // Extra high-frequency filaments
    float filaments = fbm(noiseCoord * 7.5 + t * 2.2, 3);
    filaments = pow(filaments, 3.5);

    // ── Color ramp (hot plasma) ─────────────────────────────────────────────
    // Deep violet → electric blue → cyan → white-hot
    vec3 colDeep  = vec3(0.15, 0.02, 0.35);
    vec3 colMid   = vec3(0.05, 0.25, 0.95);
    vec3 colHot   = vec3(0.15, 0.85, 1.00);
    vec3 colCore  = vec3(1.00, 0.95, 0.85);

    float heat = field * 1.15 + filaments * 0.65;
    heat = clamp(heat, 0.0, 1.5);

    vec3 plasmaCol = mix(colDeep, colMid, smoothstep(0.0, 0.45, heat));
    plasmaCol = mix(plasmaCol, colHot,  smoothstep(0.35, 0.85, heat));
    plasmaCol = mix(plasmaCol, colCore, smoothstep(0.75, 1.25, heat));

    // Tint with the instance color
    plasmaCol *= mix(vec3(1.0), fragColor.rgb, 0.55);

    // ── Edge / Fresnel energy ────────────────────────────────────────────────
    float fresnel = pow(1.0 - viewDot, 3.2);
    float rim = fresnel * (0.6 + 0.4 * sin(t * 4.0 + field * 12.0));

    // Soft outer glow (screen-space-ish falloff from the billboard)
    float screenDist = length(fragWorldPos - fragCenter) / (fragRadius * 1.35);
    float softGlow = 1.0 - smoothstep(0.75, 1.05, screenDist);
    softGlow = pow(softGlow, 1.8);

    // ── Internal volumetric feel ─────────────────────────────────────────────
    // Cheap “thickness” approximation
    float thickness = max(0.0, tFar - max(tNear, 0.0)) / (2.0 * fragRadius);
    float core = exp(-r * r * 4.5) * (0.7 + 0.3 * field);
    core *= thickness;

    // ── Final composition ───────────────────────────────────────────────────
    vec3 finalCol = plasmaCol * (0.55 + 0.45 * field);

    // Add bright filaments
    finalCol += colCore * filaments * 1.4;

    // Core bloom
    finalCol += colHot * core * 1.8;

    // Rim energy
    finalCol += mix(colMid, colHot, 0.6) * rim * 1.6;

    // Soft outer aura
    finalCol += plasmaCol * softGlow * 0.45;

    // Subtle time-based flicker
    float flicker = 0.92 + 0.08 * sin(t * 17.0 + field * 30.0);
    finalCol *= flicker;

    // Alpha: opaque-ish core, soft edges
    float alpha = smoothstep(1.05, 0.55, r);
    alpha = max(alpha, softGlow * 0.35);
    alpha = clamp(alpha * (0.75 + 0.25 * field), 0.0, 1.0);
    alpha *= fragColor.a;

    // Optional boost from extra.w (useful as a “power” multiplier)
    finalCol *= 1.0 + fragExtra.w * 0.8;
    alpha    *= 1.0 + fragExtra.w * 0.3;

    outColor = vec4(finalCol, alpha);
}