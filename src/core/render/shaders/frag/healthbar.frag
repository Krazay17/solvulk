#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;
layout(location = 2) in vec4 fragExtra; // extra.x = fill percentage (0.0 to 1.0)
layout(location = 3) flat in uint fragTextureId;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform Game {
    double gameTime;
} game;

void main() {
    float fill = clamp(fragExtra.x, 0.0, 1.0);
    float t    = float(game.gameTime);

    // Traveling wave pattern
    float wave1 = sin(fragUV.x * 8.0 - t * 2.5 + fragUV.y * 2.0);
    float wave2 = cos(fragUV.x * 16.0 + t * 1.8 - fragUV.y * 4.0);
    float energy = pow((wave1 + wave2 * 0.5) * 0.25 + 0.5, 1.5);

    // Color compositions
    vec3 filledColor = mix(fragColor.rgb * 0.85, fragColor.rgb * 1.45, energy);
    vec3 emptyColor  = fragColor.rgb * 0.15;

    // Single-pass fill mask
    float isFilled = step(fragUV.x, fill);
    vec3 baseColor = mix(emptyColor, filledColor, isFilled);

    // Subtle edge vignette
    vec2 edge = smoothstep(vec2(0.0), vec2(0.04), fragUV) * 
                 smoothstep(vec2(1.0), vec2(0.96), fragUV);
    float border = edge.x * edge.y;

    outColor = vec4(baseColor * mix(0.6, 1.0, border), fragColor.a);
}