#version 450

layout(set = 1, binding = 0) uniform Scene {
    mat4 viewProjection;
    mat4 view;
    mat4 proj;
    vec4 cameraPos;
    vec4 sun;
} scene;

struct Sphere {
    vec4 pos;       // xyz = world position, w = radius
    vec4 color;
    vec4 params;    // params.x = glow intensity (optional)
    uint kind;
    uint _padding[3];
};

layout(set = 2, binding = 0) readonly buffer Spheres {
    Sphere spheres[];
};

const vec2 CORNERS[6] = vec2[](
    vec2(-1, -1), vec2( 1, -1), vec2( 1,  1),
    vec2(-1, -1), vec2( 1,  1), vec2(-1,  1)
);

layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec4 fragColor;
layout(location = 2) out vec4 fragSphereCenter;  // xyz = center, w = radius
layout(location = 3) out vec3 fragRayOrigin;
layout(location = 4) out vec4 fragParams;

void main() {
    Sphere s = spheres[gl_InstanceIndex];
    vec2 corner = CORNERS[gl_VertexIndex];
    float radius = s.pos.w;
    float quadSize = radius * 1.2;

    // 1. Transform sphere center to View Space
    vec4 viewCenter = scene.view * vec4(s.pos.xyz, 1.0);

    // 2. Expand quad directly in View Space (Screen Aligned!)
    vec4 viewPos = viewCenter;
    viewPos.xy += corner * quadSize;

    // 3. Project to Clip Space
    gl_Position = scene.proj * viewPos;

    // Pass data to fragment shader...
    fragWorldPos     = (inverse(scene.view) * viewPos).xyz;
    fragSphereCenter = vec4(s.pos.xyz, radius);
    fragRayOrigin    = scene.cameraPos.xyz;
    fragColor        = s.color;
    fragParams       = s.params;
}