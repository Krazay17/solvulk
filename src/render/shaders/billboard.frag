#version 450

layout(set = 0, binding = 0) uniform Scene {
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
layout(location = 4) in vec2 fragUV;
layout(location = 5) flat in uint fragType;
layout(location = 6) in vec4 fragParams;

layout(location = 0) out vec4 outColor;

// --- SPHERE ---
vec4 renderSphere() {
    vec3 rayDir = normalize(fragWorldPos - fragRayOrigin);
    vec3 oc = fragRayOrigin - fragSphereCenter.xyz;
    float r = fragSphereCenter.w;
    
    float b = dot(oc, rayDir);
    float c = dot(oc, oc) - r * r;
    float disc = b * b - c;
    if (disc < 0.0) discard;
    
    float t = -b - sqrt(disc);
    if (t < 0.0) discard;
    
    vec3 hitPos = fragRayOrigin + rayDir * t;
    vec3 normal = normalize(hitPos - fragSphereCenter.xyz);
    
    vec4 clipPos = scene.viewProjection * vec4(hitPos, 1.0);
    gl_FragDepth = clipPos.z / clipPos.w;
    
    // Neon: brighter at silhouette
    vec3 viewDir = normalize(fragRayOrigin - hitPos);
    float fresnel = pow(1.0 - max(dot(normal, viewDir), 0.0), 2.0);
    
    vec3 core = fragColor.rgb;
    vec3 edge = fragColor.rgb * 2.0 + vec3(fresnel);
    vec3 result = mix(core, edge, fresnel) * 1.3;
    
    return vec4(result, fragColor.a);
}

// --- HEALTHBAR ---
vec4 renderHealthbar() {
    vec2 uv = fragUV;  // 0..1
    
    // Healthbar shape: wide and short, centered vertically in the quad.
    // Bar occupies the middle 30% of the quad vertically.
    float barTop    = 0.1;
    float barBottom = 0.0;
    
    // Outside the bar region — transparent
    if (uv.y < barBottom || uv.y > barTop) discard;
    
    // Border (1 pixel-ish thick)
    float borderThickness = 0.02;
    bool inBorderH = uv.y < barBottom + borderThickness || uv.y > barTop - borderThickness;
    bool inBorderV = uv.x < borderThickness || uv.x > 1.0 - borderThickness;
    
    if (inBorderH || inBorderV) {
        return vec4(0.0, 0.0, 0.0, 1.0);  // black border
    }
    
    // Inside the bar
    float fill = clamp(fragParams.x, 0.0, 1.0);
    vec3 emptyColor = vec3(0.15, 0.0, 0.0);
    vec3 fillColor  = fragColor.rgb;
    
    vec3 col = (uv.x < fill) ? fillColor : emptyColor;
    return vec4(col, 1.0);
}

// --- ICON ---
vec4 renderIcon() {
    // Circular cutout — replace with atlas sampling later
    vec2 centered = fragUV * 2.0 - 1.0;
    float dist = length(centered);
    if (dist > 1.0) discard;
    
    float alpha = smoothstep(1.0, 0.85, dist) * fragColor.a;
    return vec4(fragColor.rgb, alpha);
}

void main() {
    if (fragType == 0u) {
        outColor = renderSphere();
        // renderSphere sets gl_FragDepth from ray hit
    } else if (fragType == 1u) {
        outColor = renderHealthbar();
        gl_FragDepth = gl_FragCoord.z;  // natural depth
    } else if (fragType == 2u) {
        outColor = renderIcon();
        gl_FragDepth = gl_FragCoord.z;  // natural depth
    } else {
        outColor = vec4(1.0, 0.0, 1.0, 1.0);
        gl_FragDepth = gl_FragCoord.z;
    }
}