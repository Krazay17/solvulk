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

// Simple 3D noise/wave generator
float FireNoise(vec3 p, float t) {
    float v = 0.0;
    v += sin(p.x * 4.0 + t * 3.0) * cos(p.y * 4.0 - t * 2.0);
    v += sin(p.z * 5.0 + t * 4.0) * sin(p.x * 2.0 + t * 1.5);
    v += cos(p.y * 8.0 + t * 5.0) * cos(p.z * 7.0 - t * 3.0) * 0.5;
    return (v * 0.5) + 0.5; // Normalize roughly to [0.0, 1.0]
}

void main() {
    float fTime = float(time);
    vec3 rayDir = normalize(fragWorldPos - fragRayOrigin);
    
    // 1. WAVY BOUNDARY MATH (Displace the radius before intersecting)
    // We sample the noise based on the ray direction vector so the 
    // sphere's shell deforms dynamically in local space.
    vec3 waveSamplePos = rayDir * 2.0;
    waveSamplePos.y -= fTime * 2.0; // Wave movement velocity
    
    float waveNoise = FireNoise(waveSamplePos, fTime);
    
    // Deform the base radius by up to 15% inwards or outwards
    float baseRadius = fragSphereCenter.w;
    float waveAmplitude = baseRadius * 0.15; 
    float dynamicRadius = baseRadius + (waveNoise - 0.5) * waveAmplitude;

    // 2. Ray-Sphere Intersection using our new DYNAMIC radius
    vec3 oc = fragRayOrigin - fragSphereCenter.xyz;
    float b = dot(oc, rayDir);
    float c = dot(oc, oc) - dynamicRadius * dynamicRadius;
    float disc = b * b - c;
    if (disc < 0.0) discard;
    
    float t = -b - sqrt(disc);
    if (t < 0.0) discard;
    
    // Calculate world space surface details
    vec3 hitPos = fragRayOrigin + rayDir * t;
    
    // 3. Wavy Normals
    // Recomputing the normal using the displaced radius breaks the clean sphere shading
    vec3 normal = normalize(hitPos - fragSphereCenter.xyz);
    
    // Write correct depth for the newly distorted surface
    vec4 clipPos = scene.viewProjection * vec4(hitPos, 1.0);
    gl_FragDepth = clipPos.z / clipPos.w;
    
    // 4. Local Space + Radius Normalization
    vec3 localHitPos = (hitPos - fragSphereCenter.xyz) / dynamicRadius; 
    
    // 5. Calculate Fresnel Edge based on our wavy surface
    vec3 viewDir = normalize(fragRayOrigin - hitPos);
    float fresnel = pow(1.0 - max(dot(normal, viewDir), 0.0), 2.0);
    
    // 6. Secondary Internal Fire Noise & Dripping
    vec3 noisePos = localHitPos * 1.0;
    
    // The drip distortion pulls down harder at the wavy edges
    float dripStrength = 1.2;
    noisePos.y += fresnel * dripStrength;
    noisePos.y -= fTime * 3.0; // Continuous internal roll
    
    float rawNoise = FireNoise(noisePos, fTime);
    float noise = mix(0.3, 1.0, rawNoise); 
    
    // 7. Shading & Color Ramping
    float edgeProfile = smoothstep(0.0, 0.5, fresnel * noise);
    float fireIntensity = mix(noise, noise * 0.4, edgeProfile);
    
    vec3 baseColor   = fragColor.rgb;       
    vec3 moltenCore  = vec3(1.0, 0.4, 0.0); // Bright Orange
    vec3 superHeat   = vec3(1.5, 1.2, 0.5); // White-Hot Yellow
    
    vec3 fireColor = mix(baseColor * 0.4, baseColor, smoothstep(0.0, 0.25, fireIntensity));
    fireColor = mix(fireColor, moltenCore, smoothstep(0.25, 0.55, fireIntensity));
    fireColor = mix(fireColor, superHeat, smoothstep(0.55, 0.85, fireIntensity));
    
    // Add glowing heat highlight to the wavy borders
    fireColor += moltenCore * edgeProfile * 0.6;
    
    // 8. Soft Edge Alpha Alpha Masking
    // We already warped the geometry shell, but dropping the alpha slightly 
    // right at the extreme tips softens the raymarching rim edge cleanly.
    float alpha = fragColor.a;
    float edgeMask = smoothstep(0.98, 0.8, fresnel * (1.0 - rawNoise * 0.3));
    alpha *= edgeMask;
    
    if (alpha < 0.1) discard;
    
    outColor = vec4(fireColor, alpha);
}