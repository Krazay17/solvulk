#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 3) flat in int instanceIndex;
layout(location = 4) flat in uint flags;

layout(location = 0) out vec4 outColor;

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

// Structs matching your CPU descriptors
struct SphereInstance {
    vec4 pos;    // xyz = world center, w = base radius
    vec4 color;  
    vec4 params; // params.x = expansion progress (0.0 to 1.0)
    uint type;
    uint padding[3];
};

layout(std430, set = 2, binding = 0) readonly buffer SphereBuffer {
    SphereInstance instances[];
} sphereData;

// High-frequency fracture noise generator for sharp lines
float SharpFractureNoise(vec3 p, float t) {
    float v = 0.0;
    // Layering sine waves with high frequencies and absolute values creates sharp ridges/crevices
    v += abs(sin(p.x * 6.0 + t * 4.0) * cos(p.y * 6.0 - t * 3.0));
    v += abs(sin(p.z * 8.0 + t * 5.0) * sin(p.x * 4.0 + t * 2.0)) * 0.5;
    v += abs(cos(p.y * 12.0) * cos(p.z * 10.0)) * 0.25;
    
    // Invert it so the "ridges" become thin bright threads
    return 1.0 - (v / 1.75); 
}

void main() {
    float fTime = float(time);
    
    // Grab instance specific data using the mapped index
    SphereInstance sphere = sphereData.instances[instanceIndex];
    vec3 sphereCenter = sphere.pos.xyz;
    float baseRadius  = sphere.pos.w;
    
    // Impact expansion progress (0.0 at center, fading out as it reaches 1.0)
    float expansion = clamp(sphere.params.x, 0.0, 1.0);
    
    // The sphere exponentially inflates over time based on CPU expansion parameters
    float currentRadius = baseRadius * (1.0 + expansion * 2.0);

    // 1. Procedural Raycasting Boundary
    vec3 rayOrigin = scene.cameraPos.xyz;
    vec3 rayDir = normalize(fragWorldPos - rayOrigin);
    
    vec3 oc = rayOrigin - sphereCenter;
    float b = dot(oc, rayDir);
    float c = dot(oc, oc) - currentRadius * currentRadius;
    float disc = b * b - c;
    
    // If ray misses our expansion boundary, clip it
    if (disc < 0.0) discard;
    
    float hitT = -b - sqrt(disc);
    if (hitT < 0.0) discard;
    
    vec3 hitPos = rayOrigin + rayDir * hitT;
    vec3 normal = normalize(hitPos - sphereCenter);
    
    // Write dynamic depth footprint
    vec4 clipPos = scene.viewProjection * vec4(hitPos, 1.0);
    gl_FragDepth = clipPos.z / clipPos.w;
    
    // 2. Local Space Normalization
    vec3 localPos = (hitPos - sphereCenter) / currentRadius;
    
    // 3. Generating Fire Lines with Sharp Crevices
    // Radiating outward from impact center: scale up sample coordinates over expansion time
    vec3 noiseSamplePos = localPos * (4.0 + expansion * 3.0);
    
    // Add rapid, violent temporal fluctuation
    float lineNoise = SharpFractureNoise(noiseSamplePos, fTime * 2.5);
    
    // 4. Forcing Gaps (The Threshold Cut)
    // As the explosion expands, the "gaps" widen by lifting the threshold floor
    float cutThreshold = mix(0.55, 0.75, expansion); 
    
    // If the noise value is low, it's a completely cold gap -> DISCARD!
    if (lineNoise < cutThreshold) discard;
    
    // Normalize the surviving values to a [0.0, 1.0] intensity spectrum
    float flameIntensity = smoothstep(cutThreshold, 1.0, lineNoise);
    
    // 5. Fresnel Edge Thinning
    // Makes the fire lines look like three-dimensional arcs leaping through the air
    vec3 viewDir = normalize(scene.cameraPos.xyz - hitPos);
    float edgeMask = max(dot(normal, viewDir), 0.0);
    
    // 6. Thermal Color Mapping
    vec3 outerTendril = sphere.color.rgb;   // Custom base tint (e.g. Pure Red or Orange)
    vec3 plasmaCore   = vec3(1.5, 0.6, 0.1); // Bright Searing Orange
    vec3 lightningCore = vec3(2.0, 2.0, 1.5); // Searing White center core
    
    vec3 finalFireColor = mix(outerTendril * 0.3, plasmaCore, smoothstep(0.0, 0.4, flameIntensity));
    finalFireColor = mix(finalFireColor, lightningCore, smoothstep(0.6, 0.95, flameIntensity));
    
    // 7. Dynamic Dissipation Fading
    // The fire lines burn out and turn to ash (alpha fades) near the end of the expansion window
    float lifecycleAlpha = smoothstep(1.0, 0.6, expansion);
    float alpha = flameIntensity * edgeMask * lifecycleAlpha * sphere.color.a;
    
    if (alpha < 0.05) discard;
    
    outColor = vec4(finalFireColor, alpha);
}