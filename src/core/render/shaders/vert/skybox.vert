#version 450

layout(set = 0, binding = 0) uniform Scene {
    mat4 viewProjection;
    mat4 view;
    mat4 proj;
    vec4 cameraPos;
    vec4 sun;
} scene;

layout(location = 0) out vec3 viewDir;

void main() {
    vec2 pos = vec2(
        (gl_VertexIndex == 1) ? 3.0 : -1.0,
        (gl_VertexIndex == 2) ? 3.0 : -1.0
    );
    
    gl_Position = vec4(pos, 0.999, 1.0);
    
    // Reconstruct view space ray directly from projection matrix (no GPU inverse needed)
    vec3 rayView = vec3(pos.x / scene.proj[0][0], pos.y / scene.proj[1][1], -1.0);
    
    // Rotate into world space using view transpose (exact, jitter-free inverse rotation)
    viewDir = transpose(mat3(scene.view)) * rayView;
}