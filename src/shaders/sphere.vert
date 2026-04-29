#version 450

layout(set = 0, binding = 0) uniform Scene {
    mat4 viewProjection;
    mat4 view;
    mat4 proj;
    vec4 cameraPos;
    vec4 sun;
} scene;

struct Sphere {
    vec4 position;   // xyz = center, w = radius
    vec4 color;
};
layout(set = 1, binding = 0) readonly buffer Spheres {
    Sphere spheres[];
};

layout(location = 0) out vec3 fragWorldPos;     // billboard corner in world space
layout(location = 1) out vec4 fragColor;
layout(location = 2) out vec4 fragSphereCenter; // xyz=center, w=radius
layout(location = 3) out vec3 fragRayOrigin;    // camera position

void main() {
    Sphere s = spheres[gl_InstanceIndex];
    
    // Six corners for the billboard quad (two tris)
    vec2 corners[6] = vec2[](
        vec2(-1, -1), vec2( 1, -1), vec2( 1,  1),
        vec2(-1, -1), vec2( 1,  1), vec2(-1,  1)
    );
    vec2 corner = corners[gl_VertexIndex];
    
    // Build a billboard facing the camera
    vec3 toCam = normalize(scene.cameraPos.xyz - s.position.xyz);
    vec3 right = normalize(cross(vec3(0, 1, 0), toCam));
    vec3 up    = cross(toCam, right);
    
    vec3 worldCorner = s.position.xyz 
                     + right * corner.x * s.position.w 
                     + up    * corner.y * s.position.w;
    
    fragWorldPos     = worldCorner;
    fragColor        = s.color;
    fragSphereCenter = s.position;
    fragRayOrigin    = scene.cameraPos.xyz;
    
    gl_Position = scene.viewProjection * vec4(worldCorner, 1.0);
}