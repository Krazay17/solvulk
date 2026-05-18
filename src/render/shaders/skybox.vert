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
    
    gl_Position = vec4(pos, 0.999, 1.0);  // doesn't matter with depthTest off
    
    // Strip translation from view
    mat4 viewNoTrans = scene.view;
    viewNoTrans[3] = vec4(0, 0, 0, 1);
    mat4 invVP = inverse(scene.proj * viewNoTrans);
    
    vec4 worldDir = invVP * vec4(pos, 1.0, 1.0);
    viewDir = worldDir.xyz / worldDir.w;
}