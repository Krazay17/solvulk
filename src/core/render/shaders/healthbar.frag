#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;
layout(location = 2) in vec4 fragExtra;
layout(location = 3) flat in uint fragType;
layout(location = 4) flat in uint fragTextureId;
layout(location = 5) flat in uint fragFlags;

layout(location = 0) out vec4 outColor;

// --- HEALTHBAR RENDER R0UTINE ---
vec4 renderHealthbar() {
    vec2 uv = fragUV;  
    
    // Border Thickness from extra.y
    float borderThickness = fragExtra.y;
    bool inBorderV = uv.x < borderThickness || uv.x > 1.0 - borderThickness;
    bool inBorderH = uv.y < borderThickness || uv.y > 1.0 - borderThickness;
    
    if (inBorderH || inBorderV) {
        return vec4(0.0, 0.0, 0.0, 1.0); // Black outer border frame
    }
    
    // Fill Value from extra.x
    float fill = clamp(fragExtra.z, 0.0, 1.0);
    vec3 emptyColor = vec3(0.15, 0.0, 0.0); // Deep red background fill
    vec3 fillColor  = fragColor.rgb;        // Core life bar color passed via CPU
    
    vec3 col = (uv.x < fill) ? fillColor : emptyColor;
    return vec4(col, 1.0);
}

void main() {
    // This fragment shader is bound to your health pipeline pass
    outColor = renderHealthbar();
}