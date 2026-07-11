#version 450
layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;
layout(set = 2, binding = 0) uniform sampler2D textures[1];

layout(location = 0) out vec4 outColor;

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

void main() {
    vec3 msdf = texture(textures[0], fragUV).rgb;
    
    // Signed distance: positive = inside glyph, negative = outside.
    float sd = median(msdf.r, msdf.g, msdf.b) - 0.5;
    
    // Anti-aliased coverage from screen-space derivative.
    // fwidth gives the rate of change per pixel — divides the SD into
    // a smooth alpha ramp exactly one pixel wide at the glyph edge.
    float coverage = clamp(sd / fwidth(sd) + 0.5, 0.0, 1.0);
    
    if (coverage <= 0.0) discard;
    
    outColor = vec4(fragColor.rgb, fragColor.a * coverage);
}