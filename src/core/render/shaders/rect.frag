#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;
layout(location = 2) in vec2 localPos;
layout(location = 3) in vec2 rectDims;
layout(location = 4) flat in float fragBorder; // Recieved float border
layout(location = 5) flat in uint fragTextureId;
layout(location = 6) flat in uint fragFlags;

layout(set = 2, binding = 0) uniform sampler2D textures[64];

layout(location = 0) out vec4 outColor;

void main() {
    // Hollow border: discard the interior if border thickness > 0
    if (fragBorder > 0.0) {
        if (localPos.x > fragBorder && localPos.x < (rectDims.x - fragBorder) &&
            localPos.y > fragBorder && localPos.y < (rectDims.y - fragBorder)) 
        {
            discard;
        }
    }

    vec4 tex = (fragTextureId != 0u)
        ? texture(textures[fragTextureId], fragUV)
        : vec4(1.0);
    
    outColor = tex * fragColor;
}