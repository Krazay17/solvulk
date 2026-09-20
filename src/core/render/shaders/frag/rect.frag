#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;
layout(location = 2) in vec2 localPos;
layout(location = 3) in vec2 rectDims;
layout(location = 4) in vec4 fragExtra;
layout(location = 5) flat in uint fragTextureId;
layout(location = 6) flat in uint fragFlags;

layout(set = 2, binding = 0) uniform sampler2D textures[64];

layout(location = 0) out vec4 outColor;

const uint UI_FLAG_FILL          = 1u << 0;
const uint UI_FLAG_FILL_VERTICAL = 1u << 1;
const uint UI_FLAG_FILL_INVERT   = 1u << 2;

void main() {
    if ((fragFlags & UI_FLAG_FILL) != 0u) {
        float fill = clamp(fragExtra.y, 0.0, 1.0);
        vec2 normPos = localPos / rectDims;

        float progress = ((fragFlags & UI_FLAG_FILL_VERTICAL) != 0u) ? normPos.y : normPos.x;

        if ((fragFlags & UI_FLAG_FILL_INVERT) != 0u) {
            progress = 1.0 - progress;
        }

        if (progress > fill) {
            discard;
        }
    }
    // 2. Hollow Border Logic (fragExtra.x = border thickness in pixels)
    float border = fragExtra.x;
    if (border > 0.0) {
        if (localPos.x > border && localPos.x < (rectDims.x - border) &&
            localPos.y > border && localPos.y < (rectDims.y - border)) 
        {
            discard;
        }
    }

    // 3. Texture Sample & Color Output
    vec4 tex = (fragTextureId != 0u)
        ? texture(textures[fragTextureId], fragUV)
        : vec4(1.0);
        
    // Standard luminance weights for RGB
    float luminance = dot(tex.rgb, vec3(0.299, 0.587, 0.114));
    vec3 gray = vec3(luminance);
    
    // Interpolate between original color and grayscale
    vec3 color = mix(tex.rgb, gray, fragExtra.z);
    
    vec4 finalColor = vec4(color, tex.a);

    outColor = finalColor * fragColor;
}