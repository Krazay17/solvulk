#version 450

// Descriptor Set 0: Game UBO (time)
layout(set = 0, binding = 0) uniform Game {
    double gameTime;
} game;

// Descriptor Set 1: Scene UBO
layout(set = 1, binding = 0) uniform Scene {
    mat4 viewProjection;
    mat4 view;
    mat4 proj;
    vec4 cameraPos;
    vec4 sun;
} scene;

// Inputs from Quad Vertex Shader
layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;
layout(location = 2) in vec4 fragExtra;
layout(location = 3) flat in uint fragTextureId;

layout(location = 0) out vec4 outColor;

// --- Procedural Helper Functions ---

vec3 palette(float d) {
    return mix(vec3(0.2, 0.7, 0.9), vec3(1.0, 0.0, 1.0), d);
}

vec2 rotate(vec2 p, float a) {
    float c = cos(a);
    float s = sin(a);
    return p * mat2(c, s, -s, c);
}

float map(vec3 p, float iTime) {
    for (int i = 0; i < 8; ++i) {
        float t = iTime * 0.2;
        p.xz = rotate(p.xz, t);
        p.xy = rotate(p.xy, t * 1.89);
        p.xz = abs(p.xz);
        p.xz -= 0.5;
    }
    return dot(sign(p), p) / 5.0;
}

vec4 rm(vec3 ro, vec3 rd, float iTime) {
    float t = 0.0;
    vec3 col = vec3(0.0);
    float d = 0.0;
    
    for (float i = 0.0; i < 64.0; i++) {
        vec3 p = ro + rd * t;
        d = map(p, iTime) * 0.5;
        
        if (d < 0.02 || d > 100.0) {
            break;
        }
        
        col += palette(length(p) * 0.1) / (400.0 * d);
        t += d;
    }
    return vec4(col, 1.0 / (d * 100.0));
}

void main() {
    // 1. Remap quad UVs [0, 1] -> [-1, 1]
    vec2 uv = fragUV * 2.0 - 1.0;

    // 2. Circular boundary mask: discard pixels outside radius 1.0 to clip the square edges
    float distSq = dot(uv, uv);
    if (distSq > 1.0) {
        discard;
    }

    // 3. Setup local camera and ray direction
    float iTime = float(game.gameTime);
    
    vec3 ro = vec3(0.0, 0.0, -50.0);
    ro.xz = rotate(ro.xz, iTime);
    
    vec3 cf = normalize(-ro);
    vec3 cs = normalize(cross(cf, vec3(0.0, 1.0, 0.0)));
    vec3 cu = normalize(cross(cf, cs));
    
    vec3 uuv = ro + cf * 3.0 + uv.x * cs + uv.y * cu;
    vec3 rd = normalize(uuv - ro);

    // 4. Execute Raymarching algorithm
    vec4 fractalCol = rm(ro, rd, iTime);

    // 5. Apply radial edge vignette so the effect smoothly fades to 0 alpha at boundaries
    float edgeAlpha = 1.0 - smoothstep(0.7, 1.0, sqrt(distSq));

    vec3 finalColor = fractalCol.rgb * fragColor.rgb;
    float finalAlpha = clamp(fragColor.a * fractalCol.a * edgeAlpha, 0.0, 1.0);

    // Discard zero-alpha fragments to prevent writing empty blending output
    if (finalAlpha < 0.01) {
        discard;
    }

    outColor = vec4(finalColor, finalAlpha);
}