#version 450

layout(set = 0, binding = 0) uniform Scene {
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
};

layout(set = 1, binding = 0) readonly buffer Spheres {
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
    
    // Camera-facing basis from view matrix
    vec3 right = vec3(scene.view[0][0], scene.view[1][0], scene.view[2][0]);
    vec3 up    = vec3(scene.view[0][1], scene.view[1][1], scene.view[2][1]);
    
    // Quad needs margin so silhouette doesn't clip at glancing angles
    float quadSize = radius * 1.2;
    vec3 worldPos = s.pos.xyz + right * corner.x * quadSize + up * corner.y * quadSize;
    
    fragWorldPos     = worldPos;
    fragColor        = s.color;
    fragSphereCenter = vec4(s.pos.xyz, radius);
    fragRayOrigin    = scene.cameraPos.xyz;
    fragParams       = s.params;
    
    gl_Position = scene.viewProjection * vec4(worldPos, 1.0);
}