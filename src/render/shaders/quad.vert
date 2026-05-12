#version 450

layout(set = 0, binding = 0) uniform Scene {
    mat4 viewProjection;
    mat4 view;
    mat4 proj;
    vec4 cameraPos;
    vec4 sun;
} scene;

struct Quad {
    vec4 pos;       // xyz = world position, w = size
    vec4 rotation;  // quaternion (x,y,z,w)
    vec4 color;     // tint
    vec4 uv;        // xy = atlas offset, zw = atlas scale
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

// Rotate a vec3 by a quaternion (x,y,z,w)
vec3 rotateByQuat(vec4 q, vec3 v) {
    return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}

void main() {
    Quad q = quads[gl_InstanceIndex];
    
    vec2 corner = CORNERS[gl_VertexIndex];
    float size = q.pos.w;
    
    // Local quad in XY plane
    vec3 localPos = vec3(corner.x, corner.y, 0.0) * size;
    
    // Apply rotation
    vec3 worldOffset = rotateByQuat(q.rotation, localPos);
    vec3 worldPos = q.pos.xyz + worldOffset;
    
    // UV mapping
    vec2 uvLocal = vec2(corner.x, -corner.y) * 0.5 + 0.5;
    fragUV = q.uv.xy + uvLocal * q.uv.zw;          // map to atlas region
    fragColor = q.color;
    
    gl_Position = scene.viewProjection * vec4(worldPos, 1.0);
}