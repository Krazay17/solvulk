#version 450

struct SphereData {
    vec4 posRadius; // xyz = position, w = radius
    vec4 color;
    vec4 extra;
};

layout(std430, set = 2, binding = 0) readonly buffer SphereBuffer {
    SphereData spheres[];
};

layout(set = 1, binding = 0) uniform Scene {
    mat4 viewProj;
    mat4 view;
    mat4 proj;
    vec4 cameraPos;
    vec4 sun;
} scene;

layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec3 fragCenter;
layout(location = 2) out vec4 fragColor;
layout(location = 3) out float fragRadius;
layout(location = 4) out vec4 fragExtra;

const vec2 QUAD_OFFSETS[6] = vec2[](
    vec2(-1.0, -1.0), vec2( 1.0, -1.0), vec2(-1.0,  1.0),
    vec2(-1.0,  1.0), vec2( 1.0, -1.0), vec2( 1.0,  1.0)
);

// Scale quad size slightly higher than 1.0 to fit perspective projection silhouette
const float QUAD_PAD = 1.35;

void main() {
    SphereData sphere = spheres[gl_InstanceIndex];

    vec3 center  = sphere.posRadius.xyz;
    float radius = sphere.posRadius.w;
    vec2 offset  = QUAD_OFFSETS[gl_VertexIndex % 6];

    // Camera Right/Up vectors from View Matrix
    vec3 camRight = vec3(scene.view[0][0], scene.view[1][0], scene.view[2][0]);
    vec3 camUp    = vec3(scene.view[0][1], scene.view[1][1], scene.view[2][1]);

    // Expand quad size by QUAD_PAD factor
    vec3 worldPos = center + (camRight * offset.x + camUp * offset.y) * (radius * QUAD_PAD);

    gl_Position = scene.viewProj * vec4(worldPos, 1.0);

    fragWorldPos = worldPos;
    fragCenter   = center;
    fragColor    = sphere.color;
    fragRadius   = radius;
    fragExtra    = sphere.extra;
}