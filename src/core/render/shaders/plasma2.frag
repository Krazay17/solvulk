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


// ============================================================
// HASH
// ============================================================

float hash11(float p)
{
    p = fract(p * 0.1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}

float hash21(vec2 p)
{
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);

    return fract(p.x * p.y);
}

float hash31(vec3 p)
{
    p = fract(p * 0.1031);
    p += dot(p, p.yzx + 33.33);

    return fract(
        (p.x + p.y) * p.z
    );
}


// ============================================================
// VALUE NOISE
// ============================================================

float noise3(vec3 p)
{
    vec3 i = floor(p);
    vec3 f = fract(p);

    f = f * f * (3.0 - 2.0 * f);

    float a = hash31(i);
    float b = hash31(i + vec3(1, 0, 0));
    float c = hash31(i + vec3(0, 1, 0));
    float d = hash31(i + vec3(1, 1, 0));

    float e = hash31(i + vec3(0, 0, 1));
    float f1 = hash31(i + vec3(1, 0, 1));
    float g = hash31(i + vec3(0, 1, 1));
    float h = hash31(i + vec3(1, 1, 1));

    float x1 = mix(a, b, f.x);
    float x2 = mix(c, d, f.x);
    float x3 = mix(e, f1, f.x);
    float x4 = mix(g, h, f.x);

    return mix(
        mix(x1, x2, f.y),
        mix(x3, x4, f.y),
        f.z
    );
}


float fbm(vec3 p)
{
    float value = 0.0;
    float amplitude = 0.5;

    for (int i = 0; i < 5; i++)
    {
        value += noise3(p) * amplitude;

        p *= 2.02;
        p += vec3(17.1, 9.2, 13.7);

        amplitude *= 0.5;
    }

    return value;
}


// ============================================================
// RAY / SPHERE
// ============================================================

bool raySphere(
    vec3 ro,
    vec3 rd,
    vec3 center,
    float radius,
    out float tNear,
    out float tFar
)
{
    vec3 oc = ro - center;

    float b = dot(oc, rd);
    float c = dot(oc, oc) - radius * radius;

    float h = b * b - c;

    if (h < 0.0)
        return false;

    h = sqrt(h);

    tNear = -b - h;
    tFar  = -b + h;

    return tFar > 0.0;
}


// ============================================================
// ELECTRIC PULSE
//
// Creates irregular temporal pulses instead of a uniform
// sine-wave brightness.
// ============================================================

float electricPulse(
    float seed,
    float t
)
{
    // Each seed gets a different frequency.
    float frequency =
        mix(1.2, 4.5, hash11(seed));

    // Different phase per filament.
    float phase =
        hash11(seed + 17.31) * 20.0;

    float wave =
        sin(
            t * frequency +
            phase
        );

    // Sharpen into electrical bursts.
    wave =
        smoothstep(
            0.55,
            0.98,
            wave
        );

    // Random-looking secondary modulation.
    float modulation =
        sin(
            t * frequency * 3.71 +
            seed * 8.2
        );

    modulation =
        modulation * 0.5 + 0.5;

    return wave * mix(
        0.55,
        1.0,
        modulation
    );
}


// ============================================================
// 2D ELECTRIC FILAMENT
//
// p.x = path coordinate
// p.y = distance from path
// ============================================================

float lightningLine(
    vec2 p,
    float seed,
    float t
)
{
    float x = p.x;

    // Large jagged displacement.
    float distortion =
        sin(
            x * 17.0 +
            seed * 13.7 +
            t * 3.0
        ) * 0.08;

    distortion +=
        sin(
            x * 41.0 -
            seed * 9.3 +
            t * 7.0
        ) * 0.035;

    distortion +=
        sin(
            x * 91.0 +
            seed * 21.0 -
            t * 4.0
        ) * 0.012;

    // Noise makes the path less mathematically periodic.
    distortion +=
        (noise3(
            vec3(
                x * 5.0,
                seed,
                t * 0.5
            )
        ) - 0.5) * 0.12;

    float distanceToBolt =
        abs(
            p.y - distortion
        );

    // Main thin bolt.
    float bolt =
        exp(
            -distanceToBolt *
            distanceToBolt *
            1800.0
        );

    // Wider electric aura.
    float aura =
        exp(
            -distanceToBolt *
            distanceToBolt *
            90.0
        );

    // Irregular pulse.
    float pulse =
        electricPulse(
            seed,
            t
        );

    // Spatial pulse variation.
    float localPulse =
        0.55 +
        0.45 *
        sin(
            x * 22.0 +
            t * 9.0 +
            seed
        );

    localPulse =
        localPulse * 0.5 + 0.5;

    return (
        bolt * 5.0 +
        aura * 1.5
    ) *
    pulse *
    localPulse;
}


// ============================================================
// BRANCHING LIGHTNING
// ============================================================

float lightning(
    vec3 p,
    float t
)
{
    float result = 0.0;

    // --------------------------------------------------------
    // Rotate the sphere coordinates so the lightning doesn't
    // align with world axes.
    // --------------------------------------------------------

    vec3 q = p;

    float a = t * 0.13;

    mat2 rot = mat2(
        cos(a), -sin(a),
        sin(a),  cos(a)
    );

    q.xz = rot * q.xz;

    // --------------------------------------------------------
    // Main electrical channels
    // --------------------------------------------------------

    for (int i = 0; i < 7; i++)
    {
        float fi = float(i);

        float angle =
            fi / 7.0 *
            6.2831853;

        float radius =
            mix(
                0.05,
                0.65,
                hash11(fi + 3.2)
            );

        vec2 center =
            vec2(
                cos(angle),
                sin(angle)
            ) * radius;

        vec2 uv =
            vec2(
                q.y,
                length(
                    q.xz - center
                ) - radius
            );

        float bolt =
            lightningLine(
                uv,
                fi + 12.73,
                t
            );

        result += bolt;
    }

    // --------------------------------------------------------
    // Diagonal internal arcs
    // --------------------------------------------------------

    vec2 diag =
        vec2(
            q.x + q.y * 0.55,
            q.z - q.y * 0.35
        );

    result +=
        lightningLine(
            diag,
            31.7,
            t * 1.15
        );

    result +=
        lightningLine(
            diag.yx,
            42.1,
            t * 0.83
        );

    // --------------------------------------------------------
    // Fine chaotic electrical web
    // --------------------------------------------------------

    float web =
        abs(
            sin(
                q.x * 35.0 +
                sin(q.y * 18.0) * 4.0 +
                t * 5.0
            )
        );

    web *=
        abs(
            sin(
                q.z * 29.0 -
                q.y * 13.0 -
                t * 3.0
            )
        );

    web =
        smoothstep(
            0.92,
            1.0,
            web
        );

    result += web * 1.5;

    return result;
}


// ============================================================
// ELECTRIC SURFACE ARCS
// ============================================================

float surfaceLightning(
    vec3 normal,
    float t
)
{
    vec3 p = normal;

    // Convert to spherical coordinates.
    float longitude =
        atan(
            p.z,
            p.x
        );

    float latitude =
        asin(
            clamp(
                p.y,
                -1.0,
                1.0
            )
        );

    float result = 0.0;

    // Several independently moving latitude/longitude bolts.
    for (int i = 0; i < 5; i++)
    {
        float fi = float(i);

        float wave =
            sin(
                longitude * (
                    5.0 +
                    hash11(fi) * 8.0
                )
                +
                latitude * 9.0
                +
                t * (
                    1.5 +
                    hash11(fi + 4.0) * 3.0
                )
                +
                sin(latitude * 11.0)
            );

        float line =
            exp(
                -abs(wave) * 12.0
            );

        float pulse =
            electricPulse(
                fi * 13.71,
                t
            );

        result +=
            line *
            pulse;
    }

    // Distorted noisy surface crack network.
    vec3 np =
        normal * 12.0;

    np += vec3(
        t * 0.8,
        -t * 0.55,
        t * 0.65
    );

    float n =
        fbm(np);

    float cracks =
        smoothstep(
            0.72,
            0.92,
            n
        );

    result +=
        cracks *
        electricPulse(
            91.3,
            t
        );

    return result;
}


// ============================================================
// MAIN
// ============================================================

void main()
{
    float t = float(time);

    vec3 ro =
        scene.cameraPos.xyz;

    vec3 rd =
        normalize(
            fragWorldPos - ro
        );

    vec3 center =
        fragCenter;

    float radius =
        fragRadius;

    // --------------------------------------------------------
    // Sphere intersection
    // --------------------------------------------------------

    float tNear;
    float tFar;

    if (!raySphere(
        ro,
        rd,
        center,
        radius,
        tNear,
        tFar))
    {
        discard;
    }

    tNear =
        max(
            tNear,
            0.0
        );

    vec3 hit =
        ro + rd * tNear;

    vec3 normal =
        normalize(
            hit - center
        );

    vec3 p =
        (hit - center) /
        radius;

    // --------------------------------------------------------
    // Fresnel
    // --------------------------------------------------------

    float facing =
        max(
            dot(
                normal,
                -rd
            ),
            0.0
        );

    float fresnel =
        pow(
            1.0 - facing,
            4.0
        );

    // --------------------------------------------------------
    // INTERNAL LIGHTNING
    // --------------------------------------------------------

    float bolts =
        lightning(
            p,
            t
        );

    // --------------------------------------------------------
    // SURFACE LIGHTNING
    // --------------------------------------------------------

    float surface =
        surfaceLightning(
            normal,
            t
        );

    // --------------------------------------------------------
    // Chaotic global electrical activity
    // --------------------------------------------------------

    float chaos =
        fbm(
            p * 9.0 +
            vec3(
                t * 1.7,
                -t * 1.1,
                t * 1.35
            )
        );

    // Non-uniform electrical activity.
    float energy =
        bolts * (
            0.55 +
            chaos * 0.9
        );

    energy +=
        surface *
        1.8;

    // --------------------------------------------------------
    // Irregular whole-orb pulse
    //
    // This is deliberately NOT a smooth global sine pulse.
    // --------------------------------------------------------

    float pulseA =
        electricPulse(
            3.7,
            t
        );

    float pulseB =
        electricPulse(
            19.3,
            t * 0.73
        );

    float globalPulse =
        max(
            pulseA,
            pulseB * 0.7
        );

    // --------------------------------------------------------
    // Electrical core
    // --------------------------------------------------------

    float core =
        pow(
            facing,
            2.5
        );

    core *=
        0.6 +
        chaos * 0.8;

    // --------------------------------------------------------
    // Edge electrical corona
    // --------------------------------------------------------

    float corona =
        fresnel *
        (
            0.35 +
            chaos * 1.5
        );

    corona *=
        0.5 +
        globalPulse;

    // --------------------------------------------------------
    // COLOR
    // --------------------------------------------------------

    vec3 base =
        fragColor.rgb;

    // Dark charged body.
    vec3 dark =
        base * 0.025;

    // Electric blue.
    vec3 electric =
        mix(
            base,
            vec3(
                0.05,
                0.35,
                1.0
            ),
            0.65
        );

    // Hot blue-white.
    vec3 hot =
        vec3(
            0.55,
            0.85,
            1.0
        );

    // White-hot lightning.
    vec3 white =
        vec3(
            1.0,
            1.0,
            1.0
        );

    // Base orb.
    vec3 color =
        mix(
            dark,
            electric,
            chaos * 0.75
        );

    // Internal electrical energy.
    color +=
        electric *
        energy *
        0.55;

    // Hot portions of the bolts.
    color +=
        hot *
        smoothstep(
            1.0,
            3.0,
            energy
        ) *
        1.5;

    // White-hot electrical cores.
    color +=
        white *
        smoothstep(
            2.5,
            6.0,
            energy
        ) *
        3.0;

    // Bright core.
    color +=
        hot *
        core *
        (
            0.5 +
            globalPulse
        );

    // Electric corona.
    color +=
        vec3(
            0.1,
            0.55,
            1.0
        ) *
        corona *
        3.0;

    // --------------------------------------------------------
    // Rim lightning flashes
    // --------------------------------------------------------

    float rimFlash =
        surface *
        fresnel;

    color +=
        white *
        rimFlash *
        2.0;

    // --------------------------------------------------------
    // Flickering exposure
    // --------------------------------------------------------

    float flicker =
        hash11(
            floor(t * 17.0)
        );

    // Only occasionally affects the whole orb.
    float flash =
        smoothstep(
            0.78,
            0.98,
            flicker
        );

    color +=
        hot *
        flash *
        0.35;

    // --------------------------------------------------------
    // Alpha
    // --------------------------------------------------------

    float alpha =
        0.72 +
        fresnel * 0.28;

    // Lightning makes parts of the orb more opaque.
    alpha +=
        smoothstep(
            0.7,
            3.0,
            energy
        ) * 0.25;

    alpha =
        clamp(
            alpha,
            0.0,
            1.0
        );

    // --------------------------------------------------------
    // HDR intensity
    // --------------------------------------------------------

    color *= 1.25;

    outColor =
        vec4(
            color,
            alpha
        );
}
