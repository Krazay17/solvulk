#version 450

layout(set = 0, binding = 0) uniform Util {
    double time;
};
layout(set = 1, binding = 0) uniform Scene {
    mat4 viewProjection;
    mat4 view;
    mat4 proj;
    vec4 cameraPos;
    vec4 sun;
} scene;


layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec4 fragColor;
layout(location = 2) in vec4 fragSphereCenter;
layout(location = 3) in vec3 fragRayOrigin;
layout(location = 4) in vec4 fragParams;

layout(location = 0) out vec4 outColor;

void main() {
    // Ray from camera through this fragment's world position
    vec3 rayDir = normalize(fragWorldPos - fragRayOrigin);
    vec3 oc = fragRayOrigin - fragSphereCenter.xyz;
    float r = fragSphereCenter.w;
    
    // Sphere intersection
    float b = dot(oc, rayDir);
    float c = dot(oc, oc) - r * r;
    float disc = b * b - c;
    if (disc < 0.0) discard;
    
    float t = -b - sqrt(disc);
    if (t < 0.0) discard;
    
    vec3 hitPos = fragRayOrigin + rayDir * t;
    vec3 normal = normalize(hitPos - fragSphereCenter.xyz);
    
    // Write correct depth for the sphere surface
    vec4 clipPos = scene.viewProjection * vec4(hitPos, 1.0);
    gl_FragDepth = clipPos.z / clipPos.w;
    
    // Neon glow effect
    vec3 viewDir = normalize(fragRayOrigin - hitPos);
    float fresnel = pow(1.0 - max(dot(normal, viewDir), 0.0), 2.0);
    
    vec3 core = fragColor.rgb;
    vec3 edge = fragColor.rgb * 2.0 + vec3(fresnel);
    vec3 result = mix(core, edge, fresnel) * 1.3;
    
    outColor = vec4(result, fragColor.a);
}