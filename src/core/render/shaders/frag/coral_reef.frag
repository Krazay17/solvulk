#version 450

layout(location = 0) in vec3 v_rayDir; // Interpolated 3D world/view ray direction from vertex shader

layout(location = 0) out vec4 fragColor;

layout(set = 2, binding = 0) uniform Game {
    double u_time;
}game;

#define r(a) mat2(cos(a - vec4(0, 11, 33, 0)))

void main() {
    // 1. Normalize the 3D direction vector from the camera to skybox geometry
    vec3 rd = normalize(v_rayDir);

    vec3 P, Q;
    float i = 0.0, g = 0.0, d = 0.1, a;

    vec4 o = vec4(0.0);

    for(; i < 99.0 && d > 1e-4; i++) {
        // 2. Sample along 3D ray direction vector instead of 2D screen UVs
        g += d * 0.3;
        P = rd * g;

        // Apply tunnel motion & rotation
        P.z += float(game.u_time * 0.2);
        P.xy *= r(P.z * 0.8);

        // Base distance field
        d = 1.0 - abs(P.y);

        // Repeated fractal structure
        for(a = 2.0; a < 6e2; a += a) {
            Q = P * a;
            Q.z += d * 12.5;
            d -= abs(dot(sin(Q), Q - Q + 1.0)) / (a * 3.0);
        }

        // Color accumulation
        o += (0.5 + 0.5 * cos(d * 40.0 + P.z * 2.0 + vec4(1.0, 4.4, 4.0, 0.0))) / (0.006 + abs(d) * 0.08);
    }

    // Tone mapping (scaled by 3D direction vector length instead of 2D screen length)
    o = tanh(o / 2e4 / length(rd.xy));

    fragColor = o;
}