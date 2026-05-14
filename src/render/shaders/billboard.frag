#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;
layout(location = 2) flat in uint fragType;
layout(location = 3) in vec4 fragParams;

layout(location = 0) out vec4 outColor;

// --- HEALTHBAR ---
vec4 renderHealthbar() {
    vec2 uv = fragUV;  // 0..1
    
    // Bar occupies the middle vertical strip of the quad
    float barTop    = 0.55;
    float barBottom = 0.45;
    
    if (uv.y < barBottom || uv.y > barTop) discard;
    
    // Border
    float borderThickness = fragParams.y;
    bool inBorderV = uv.x < borderThickness || uv.x > 1.0 - borderThickness;
    bool inBorderH = uv.y < barBottom + borderThickness || uv.y > barTop - borderThickness;
    
    if (inBorderH || inBorderV) {
        return vec4(0.0, 0.0, 0.0, 1.0);
    }
    
    // Fill
    float fill = clamp(fragParams.x, 0.0, 1.0);
    vec3 emptyColor = vec3(0.15, 0.0, 0.0);
    vec3 fillColor  = fragColor.rgb;
    
    vec3 col = (uv.x < fill) ? fillColor : emptyColor;
    return vec4(col, 1.0);
}

// --- ICON ---
vec4 renderIcon() {
    // Circular cutout placeholder.
    // Later: sample from an icon atlas using fragUV + fragParams as atlas coords.
    vec2 centered = fragUV * 2.0 - 1.0;
    float dist = length(centered);
    if (dist > 1.0) discard;
    
    float alpha = smoothstep(1.0, 0.85, dist) * fragColor.a;
    return vec4(fragColor.rgb, alpha);
}

void main() {
    if (fragType == 0u) {
        outColor = renderHealthbar();
    } else if (fragType == 1u) {
        outColor = renderIcon();
    } else {
        outColor = vec4(1.0, 0.0, 1.0, 1.0);  // magenta = bug
    }
}