#version 450

layout(set = 0, binding = 0) uniform Scene {
    mat4 viewProjection;
    mat4 view;
    mat4 proj;
    vec4 cameraPos;
    vec4 sun;
} scene;

struct Quad {
    vec4 pos;       // xyz + size
    vec4 rotation;  // quaternion OR rotation.x = spin angle (radians)
    vec4 color;
    vec4 uv;
    uint type;
    uint _pad[3];
};

layout(set = 1, binding = 0) readonly buffer Quads {
    Quad quads[];
};

const vec2 CORNERS[6] = vec2[](
    vec2(-1, -1), vec2( 1, -1), vec2( 1,  1),
    vec2(-1, -1), vec2( 1,  1), vec2(-1,  1)
);

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec4 fragColor;

vec3 rotateByQuat(vec4 q, vec3 v) {
    return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}

void main() {
    Quad q = quads[gl_InstanceIndex];
    
    vec2 corner = CORNERS[gl_VertexIndex];
    float size = q.pos.w;
    vec3 worldPos;
    
    if (q.type == 0u) {
        // QUAD_CAMERAFACE — camera-facing with optional 2D spin
        float angle = q.rotation.x;
        float c = cos(angle);
        float s = sin(angle);
        vec2 spun = vec2(corner.x * c - corner.y * s,
                         corner.x * s + corner.y * c);
        
        vec3 right = vec3(scene.view[0][0], scene.view[1][0], scene.view[2][0]);
        vec3 up    = vec3(scene.view[0][1], scene.view[1][1], scene.view[2][1]);
        
        worldPos = q.pos.xyz + right * spun.x * size + up * spun.y * size;
    } else {
        // QUAD_3D — arbitrary rotation via quaternion
        vec3 localPos = vec3(corner.x, corner.y, 0.0) * size;
        worldPos = q.pos.xyz + rotateByQuat(q.rotation, localPos);
    }
    
    vec2 uvLocal = vec2(corner.x, -corner.y) * 0.5 + 0.5;   // Y-flip for texture
    fragUV = q.uv.xy + uvLocal * q.uv.zw;
    fragColor = q.color;
    
    gl_Position = scene.viewProjection * vec4(worldPos, 1.0);
}