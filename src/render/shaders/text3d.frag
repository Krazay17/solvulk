#version 450
layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;
layout(location = 2) in vec4 fragExtra;
layout(location = 3) flat in uint fragType;
layout(location = 4) flat in uint fragTextureId;
layout(location = 5) flat in uint fragFlags;

layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform sampler2D textures[64];

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

// void main() {
//     vec4 msdf = texture(textures[fragTextureId], fragUV);
//     float sd = median(msdf.r, msdf.g, msdf.b);
    
//     // Screen-space derivative for crisp edges at any scale
//     float pxRange = 4.0;   // tune to your atlas's pxRange config
//     float screenPxDistance = pxRange * (sd - 0.5);
//     float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
    
//     if (opacity < 0.01) discard;
//     outColor = vec4(fragColor.rgb, fragColor.a * opacity);
// }
void main() {
    outColor = vec4(1,0,0,1);
}