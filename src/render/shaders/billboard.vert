#version 450

layout(set = 0, binding = 0) uniform Scene {
    mat4 viewProjection;
    mat4 view;
    mat4 proj;
    vec4 cameraPos;
    vec4 sun;
} scene;

struct Billboard {
    vec4 pos;       // xyz = world pos, w = size
    vec4 color;
    vec4 params;
    uint type;
    uint _padding[3];
};

layout(set = 1, binding = 0) readonly buffer Billboards {
    Billboard billboards[];
};

const vec2 CORNERS[6] = vec2[](
    vec2(-1, -1), vec2( 1, -1), vec2( 1,  1),
    vec2(-1, -1), vec2( 1,  1), vec2(-1,  1)
);

layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec4 fragColor;
layout(location = 2) out vec4 fragSphereCenter;
layout(location = 3) out vec3 fragRayOrigin;
layout(location = 4) out vec2 fragUV;
layout(location = 5) flat out uint fragType;
layout(location = 6) out vec4 fragParams;

void main() {
    Billboard b = billboards[gl_InstanceIndex];
    
    vec2 corner = CORNERS[gl_VertexIndex];
    float size = b.pos.w;
    
    // Camera-facing basis from view matrix
    vec3 right = vec3(scene.view[0][0], scene.view[1][0], scene.view[2][0]);
    vec3 up    = vec3(scene.view[0][1], scene.view[1][1], scene.view[2][1]);
    
    // Healthbar: keep upright (yaw-only billboarding)
    if (b.type == 1u) {
        up = vec3(0.0, 1.0, 0.0);
        vec3 toCam = scene.cameraPos.xyz - b.pos.xyz;
        right = normalize(cross(up, toCam));
    }
    
    // Quad covers 2*size in world space.
    // For spheres, add 1.2x margin so silhouette doesn't clip at glancing angles.
    float scale = (b.type == 0u) ? size * 1.2 : size;
    vec3 worldPos = b.pos.xyz + right * corner.x * scale + up * corner.y * scale;
    
    fragWorldPos     = worldPos;
    fragColor        = b.color;
    fragSphereCenter = vec4(b.pos.xyz, size);
    fragRayOrigin    = scene.cameraPos.xyz;
    fragUV           = corner * 0.5 + 0.5;  // 0..1
    fragType         = b.type;
    fragParams       = b.params;
    
    gl_Position = scene.viewProjection * vec4(worldPos, 1.0);
}