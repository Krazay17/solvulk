#version 450

layout(set = 0, binding = 0) uniform Scene {
    mat4 viewProjection;
    mat4 view;
    mat4 proj;
    vec4 cameraPos;
    vec4 sun;
} scene;

struct Billboard {
    vec4 pos;       // xyz = world position, w = size
    vec4 color;
    vec4 params;    // type-specific (e.g., fill amount for healthbar)
    uint type;      // 0 = healthbar, 1 = icon, ...
    uint _padding[3];
};

layout(set = 1, binding = 0) readonly buffer Billboards {
    Billboard billboards[];
};

const vec2 CORNERS[6] = vec2[](
    vec2(-1, -1), vec2( 1, -1), vec2( 1,  1),
    vec2(-1, -1), vec2( 1,  1), vec2(-1,  1)
);

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec4 fragColor;
layout(location = 2) flat out uint fragType;
layout(location = 3) out vec4 fragParams;

void main() {
    Billboard b = billboards[gl_InstanceIndex];
    
    vec2 corner = CORNERS[gl_VertexIndex];
    float size = b.pos.w;
    
    vec3 right, up;
    
    // if (b.type == 0u) {
    //     // Healthbar: yaw-only billboarding (always upright)
    //     up = vec3(0.0, 1.0, 0.0);
    //     vec3 toCam = scene.cameraPos.xyz - b.pos.xyz;
    //     //toCam.y = 0.0;  // ignore vertical component for stable yaw
    //     right = normalize(cross(up, toCam));
    // } else {
    //     // Icon and others: fully camera-facing
    //     right = vec3(scene.view[0][0], scene.view[1][0], scene.view[2][0]);
    //     up    = vec3(scene.view[0][1], scene.view[1][1], scene.view[2][1]);
    // }
        right = vec3(scene.view[0][0], scene.view[1][0], scene.view[2][0]);
        up    = vec3(scene.view[0][1], scene.view[1][1], scene.view[2][1]);
    
    vec3 worldPos = b.pos.xyz + right * corner.x * size + up * corner.y * size;
    
    fragUV     = corner * 0.5 + 0.5;
    fragColor  = b.color;
    fragType   = b.type;
    fragParams = b.params;
    
    gl_Position = scene.viewProjection * vec4(worldPos, 1.0);
}