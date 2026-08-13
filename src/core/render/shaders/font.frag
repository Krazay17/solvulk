#version 450
layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;
layout(location = 2) in vec4 fragOutline;
layout(set = 2, binding = 0) uniform sampler2D textures[1];

layout(location = 0) out vec4 outColor;

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

void main() {
    vec3 msdf = texture(textures[0], fragUV).rgb;
    float sd = median(msdf.r, msdf.g, msdf.b) - 0.5;
    
    // Pixel range delta for crisp screen-space anti-aliasing
    float w = fwidth(sd);

    // 1. Calculate body coverage (edge at sd = 0.0)
    float bodyAlpha = clamp(sd / w + 0.5, 0.0, 1.0);

    // 2. Calculate outline coverage (edge shifted back into negative space)
    // Adjust outlineWidth (e.g., 0.1 to 0.25) depending on your atlas's MSDF range
    float outlineWidth = 0.33; 
    float outlineAlpha = clamp((sd + outlineWidth) / w + 0.5, 0.0, 1.0);

    // Early out if we are outside both the body and the outline
    if (outlineAlpha <= 0.0) discard;

    // 3. Define outline color (black with vertex alpha)
    vec4 outlineColor = vec4(0.0, 0.0, 0.0, fragColor.a);

    // 4. Layer the glyph body color over the outline color
    vec4 finalColor = mix(outlineColor, fragColor, bodyAlpha);

    // 5. Scale output alpha by overall outline mask
    outColor = vec4(finalColor.rgb, finalColor.a * outlineAlpha);
}