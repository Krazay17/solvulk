#version 450
layout(location = 0) in vec3 sphereCenterView;   // sphere center in view space
layout(location = 1) in float sphereRadius;
layout(location = 2) in vec3 rayDir;             // ray from camera through this fragment
layout(set = 0, binding = 0) uniform Scene {
    mat4 viewProj;
    mat4 view;
    mat4 proj;
    vec4 cameraPos;
    vec4 sun;
} scene;

void main() {
    // Ray-sphere intersection
    float b = dot(sphereCenterView, rayDir);
    float c = dot(sphereCenterView, sphereCenterView) - sphereRadius * sphereRadius;
    float disc = b*b - c;
    if (disc < 0.0) discard;

    float t = b - sqrt(disc);
    vec3 hitPos = rayDir * t;
    vec3 normal = normalize(hitPos - sphereCenterView);

    // Lighting
    vec3 lightDir = normalize(scene.sun);
    float diffuse = max(dot(normal, lightDir), 0.0);
    vec3 albedo = vec3(0.8, 0.3, 0.3);
    outColor = vec4(albedo * (0.2 + 0.8 * diffuse), 1.0);

    // Write depth so sphere sorts correctly with 3D geometry
    vec4 clipPos = proj * vec4(hitPos, 1.0);
    gl_FragDepth = clipPos.z / clipPos.w;
}
