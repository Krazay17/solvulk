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

#define PI 3.14159265359
#define TAU 6.28318530718

// ------------------------------------------------------------
// Hash / Noise
// ------------------------------------------------------------

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
    return fract((p.x + p.y) * p.z);
}

float noise3(vec3 p)
{
    vec3 i = floor(p);
    vec3 f = fract(p);

    f = f * f * (3.0 - 2.0 * f);

    float n000 = hash31(i + vec3(0,0,0));
    float n100 = hash31(i + vec3(1,0,0));
    float n010 = hash31(i + vec3(0,1,0));
    float n110 = hash31(i + vec3(1,1,0));

    float n001 = hash31(i + vec3(0,0,1));
    float n101 = hash31(i + vec3(1,0,1));
    float n011 = hash31(i + vec3(0,1,1));
    float n111 = hash31(i + vec3(1,1,1));

    float x00 = mix(n000, n100, f.x);
    float x10 = mix(n010, n110, f.x);
    float x01 = mix(n001, n101, f.x);
    float x11 = mix(n011, n111, f.x);

    return mix(
        mix(x00, x10, f.y),
        mix(x01, x11, f.y),
        f.z
    );
}

float fbm(vec3 p)
{
    float v = 0.0;
    float a = 0.5;

    for (int i = 0; i < 5; i++)
    {
        v += noise3(p) * a;
        p = p * 2.03 + vec3(17.1, 9.2, 13.7);
        a *= 0.5;
    }

    return v;
}

// ------------------------------------------------------------
// Sphere SDF
// ------------------------------------------------------------

float sdSphere(vec3 p, float r)
{
    return length(p) - r;
}

// ------------------------------------------------------------
// Ray / Sphere
// ------------------------------------------------------------

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

// ------------------------------------------------------------
// Fake chaotic field
// ------------------------------------------------------------

float plasmaField(vec3 p, float t)
{
    // Multiple differently rotated frequencies.
    float a = sin(
        p.x * 7.0 +
        sin(p.y * 9.0 + t * 2.1) +
        t * 3.0
    );

    float b = sin(
        p.y * 11.0 +
        cos(p.z * 8.0 - t * 2.7)
    );

    float c = sin(
        p.z * 13.0 +
        sin(p.x * 6.0 + t * 1.7)
    );

    float n = fbm(p * 4.0 + vec3(t * 0.25));

    float plasma =
        sin(
            a * 3.0 +
            b * 2.0 +
            c * 2.5 +
            n * 7.0
        );

    return plasma * 0.5 + 0.5;
}

// ------------------------------------------------------------
// Main
// ------------------------------------------------------------

void main()
{
    float t = float(time);

    vec3 ro = scene.cameraPos.xyz;

    // The interpolated quad position gives us a point on the
    // camera-facing billboard.
    vec3 rd = normalize(fragWorldPos - ro);

    vec3 center = fragCenter;
    float radius = fragRadius;

    // --------------------------------------------------------
    // Ray / sphere intersection
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

    // Start at the front surface.
    float rayT = max(tNear, 0.0);

    vec3 hitPos = ro + rd * rayT;

    vec3 normal = normalize(hitPos - center);

    // --------------------------------------------------------
    // Sphere coordinates
    // --------------------------------------------------------

    vec3 p = (hitPos - center) / radius;

    // Spherical coordinates.
    float longitude =
        atan(p.z, p.x);

    float latitude =
        asin(clamp(p.y, -1.0, 1.0));

    // --------------------------------------------------------
    // Animated distortion
    // --------------------------------------------------------

    vec3 warp = vec3(
        sin(p.y * 12.0 + t * 2.0),
        sin(p.z * 15.0 - t * 2.7),
        sin(p.x * 11.0 + t * 1.8)
    );

    vec3 distorted =
        p +
        warp * 0.075;

    // --------------------------------------------------------
    // Plasma
    // --------------------------------------------------------

    float plasma =
        plasmaField(
            distorted * 2.5,
            t
        );

    // High-frequency turbulent layer.
    float turbulence =
        fbm(
            distorted * 8.0 +
            vec3(
                t * 0.7,
                -t * 0.45,
                t * 0.3
            )
        );

    plasma = mix(
        plasma,
        turbulence,
        0.55
    );

    // Sharpen the plasma.
    plasma =
        smoothstep(
            0.25,
            0.78,
            plasma
        );

    // --------------------------------------------------------
    // Pulsing energy veins
    // --------------------------------------------------------

    float veins =
        sin(
            longitude * 17.0 +
            sin(latitude * 13.0 + t * 2.0) * 3.0 +
            t * 4.0
        );

    veins =
        smoothstep(
            0.65,
            0.95,
            veins
        );

    plasma += veins * 0.35;

    // --------------------------------------------------------
    // Fresnel
    // --------------------------------------------------------

    float facing =
        max(
            dot(normal, -rd),
            0.0
        );

    float fresnel =
        pow(
            1.0 - facing,
            3.5
        );

    // --------------------------------------------------------
    // Interior glow
    // --------------------------------------------------------

    float interior =
        pow(
            max(1.0 - length(p), 0.0),
            0.35
        );

    // --------------------------------------------------------
    // Electric rim
    // --------------------------------------------------------

    float rimNoise =
        fbm(
            normal * 12.0 +
            vec3(t * 1.5)
        );

    float electricRim =
        pow(
            fresnel,
            1.8
        ) *
        smoothstep(
            0.25,
            0.85,
            rimNoise
        );

    // --------------------------------------------------------
    // Color
    // --------------------------------------------------------

    vec3 base =
        fragColor.rgb;

    // Three-stage plasma palette.
    vec3 darkColor =
        base * 0.12;

    vec3 midColor =
        mix(
            base,
            vec3(0.15, 0.65, 1.0),
            0.35
        );

    vec3 hotColor =
        vec3(
            0.55,
            0.95,
            1.0
        );

    vec3 plasmaColor =
        mix(
            darkColor,
            midColor,
            smoothstep(
                0.05,
                0.55,
                plasma
            )
        );

    plasmaColor =
        mix(
            plasmaColor,
            hotColor,
            smoothstep(
                0.55,
                1.0,
                plasma
            )
        );

    // Add electric rim.
    plasmaColor +=
        vec3(
            0.25,
            0.75,
            1.0
        ) *
        electricRim *
        2.5;

    // Interior energy.
    plasmaColor +=
        hotColor *
        interior *
        plasma *
        0.75;

    // --------------------------------------------------------
    // Sun lighting
    // --------------------------------------------------------

    vec3 sunDir =
        normalize(scene.sun.xyz);

    float sunLight =
        max(
            dot(normal, sunDir),
            0.0
        );

    plasmaColor *=
        0.55 +
        sunLight * 0.45;

    // --------------------------------------------------------
    // Chaotic flickering
    // --------------------------------------------------------

    float flicker =
        fbm(
            p * 18.0 +
            vec3(
                t * 3.0,
                -t * 2.0,
                t * 2.5
            )
        );

    plasmaColor *=
        0.85 +
        flicker * 0.35;

    // --------------------------------------------------------
    // Bright core
    // --------------------------------------------------------

    float core =
        pow(
            max(facing, 0.0),
            1.8
        );

    plasmaColor +=
        hotColor *
        core *
        plasma *
        0.45;

    // --------------------------------------------------------
    // Alpha
    // --------------------------------------------------------

    float alpha =
        0.82 +
        fresnel * 0.18;

    // Keep the silhouette reasonably spherical.
    alpha *=
        smoothstep(
            0.0,
            0.08,
            tFar - tNear
        );

    // --------------------------------------------------------
    // HDR-style output
    // --------------------------------------------------------

    plasmaColor *= 1.35;

    outColor =
        vec4(
            plasmaColor,
            alpha
        );
}
