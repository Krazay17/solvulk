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

// ─── Minimal noise ──────────────────────────────────────────────────────────
float hash11(float p) {
    p = fract(p * 0.1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}

float hash31(vec3 p) {
    p  = fract(p * 0.1031);
    p += dot(p, p.yzx + 33.33);
    return fract((p.x + p.y) * p.z);
}

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

float fbm(vec3 p, int octaves) {
    float v = 0.0;
    float a = 0.5;
    for (int i = 0; i < octaves; ++i) {
        v += a * vnoise(p);
        p *= 2.1;
        a *= 0.5;
    }
    return v;
}

// ─── Linear / jagged bolt field ─────────────────────────────────────────────
// Creates sharp, mostly straight electric lines with abrupt breaks
float linearBolts(vec3 p, float t) {
    // Very light directional warp so bolts stay mostly linear
    vec3 q = p;
    q.x += 0.18 * sin(q.y * 4.5 + t * 3.2);
    q.y += 0.15 * sin(q.z * 5.1 - t * 2.7);
    q.z += 0.12 * sin(q.x * 3.8 + t * 4.1);

    // Primary bolt axes (different orientations)
    float b1 = abs(q.x + 0.3 * sin(q.y * 2.5 + t * 1.8));
    float b2 = abs(q.y + 0.25 * sin(q.z * 3.1 - t * 2.4));
    float b3 = abs(q.z + 0.2  * sin(q.x * 2.8 + t * 1.5));
    float b4 = abs(q.x * 0.7 + q.y * 0.7 + 0.22 * sin(q.z * 4.0 + t * 3.5));
    float b5 = abs(q.y * 0.6 - q.z * 0.8 + 0.18 * sin(q.x * 3.6 - t * 2.9));

    // Make them thin and sharp
    float bolts = 0.0;
    bolts += exp(-b1 * 28.0);
    bolts += exp(-b2 * 26.0);
    bolts += exp(-b3 * 30.0);
    bolts += exp(-b4 * 22.0) * 0.85;
    bolts += exp(-b5 * 24.0) * 0.75;

    // Jagged high-frequency breaks (makes them look electric / cracked)
    float jag = fbm(q * 9.0 + t * 2.5, 3);
    jag = pow(1.0 - abs(jag * 2.0 - 1.0), 6.0);
    bolts *= 0.55 + 0.45 * jag;

    // Strong attack pulses
    float pulse = sin(t * 9.0 + fbm(q * 2.0, 2) * 12.0);
    pulse = pow(max(pulse, 0.0), 3.5);
    bolts += pulse * 0.7 * exp(-min(b1, min(b2, b3)) * 12.0);

    return clamp(bolts, 0.0, 3.0);
}

void main() {
    float t = float(time);

    vec3 ro = scene.cameraPos.xyz;
    vec3 rd = normalize(fragWorldPos - ro);

    // Ray-sphere
    vec3 oc = ro - fragCenter;
    float b = dot(oc, rd);
    float c = dot(oc, oc) - fragRadius * fragRadius;
    float h = b * b - c;
    if (h < 0.0) discard;

    h = sqrt(h);
    float tNear = -b - h;
    float tFar  = -b + h;
    float hitT  = (tNear > 0.0) ? tNear : tFar;
    if (hitT < 0.0) discard;

    vec3 hitPos = ro + rd * hitT;
    vec3 normal = normalize(hitPos - fragCenter);

    float r = length(hitPos - fragCenter) / fragRadius;
    float viewDot = max(dot(normal, -rd), 0.0);

    // Scaled coordinates
    vec3 noiseCoord = (hitPos - fragCenter) * (3.6 / fragRadius);
    noiseCoord += fragExtra.xyz * 0.25;

    // ── Fields ──────────────────────────────────────────────────────────────
    float bolts  = linearBolts(noiseCoord, t * 1.6);
    float plasma = fbm(noiseCoord * 1.3 + t * 0.25, 3) * 0.55;

    // ── Colors ──────────────────────────────────────────────────────────────
    vec3 colDeep   = vec3(0.015, 0.03, 0.12);
    vec3 colPlasma = vec3(0.08, 0.22, 0.85);
    vec3 colBolt   = vec3(0.45, 0.80, 1.00);
    vec3 colCore   = vec3(1.00, 0.97, 0.92);

    // Dark plasma base
    vec3 col = mix(colDeep, colPlasma, plasma);

    // Aggressive linear bolts
    float boltPow = bolts * (0.75 + 0.25 * sin(t * 28.0 + bolts * 35.0));
    col += colBolt * boltPow * 2.6;
    col += colCore * pow(bolts, 2.2) * 3.8;          // pure white cores

    // Instance tint
    col *= mix(vec3(1.0), fragColor.rgb, 0.4);

    // Edge / rim
    float fresnel = pow(1.0 - viewDot, 2.6);
    col += colBolt * fresnel * (0.35 + 0.65 * bolts) * 1.3;

    // Soft outer glow
    float screenDist = length(fragWorldPos - fragCenter) / (fragRadius * 1.35);
    float softGlow = pow(1.0 - smoothstep(0.72, 1.08, screenDist), 1.7);
    col += colPlasma * softGlow * 0.4;

    // Core volume
    float thick = max(0.0, tFar - max(tNear, 0.0)) / (2.0 * fragRadius);
    float core  = exp(-r * r * 5.5) * thick;
    col += colBolt * core * 0.9;

    // Harsh electric flicker only on the bolts
    float flicker = 0.75 + 0.25 * sin(t * 42.0 + bolts * 60.0);
    col *= mix(1.0, flicker, clamp(bolts * 1.8, 0.0, 1.0));

    // Alpha
    float alpha = smoothstep(1.05, 0.48, r);
    alpha = max(alpha, softGlow * 0.35);
    alpha = max(alpha, boltPow * 0.45);          // keep bolts visible near edge
    alpha = clamp(alpha, 0.0, 1.0) * fragColor.a;

    // Intensity from extra.w
    col   *= 1.0 + fragExtra.w * 1.2;
    alpha *= 1.0 + fragExtra.w * 0.2;

    outColor = vec4(col, alpha);
}