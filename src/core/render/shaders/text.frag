#version 450
layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D textures[1];

float median(float r, float g, float b) {
    return max(min(r,g), min(max(r,g),b));
}
void main()
{
    vec3 msd = texture(textures[0], fragUV).rgb;
    float dist = median(msd.r, msd.g, msd.b);
    float alpha = smoothstep(0.49, 0.51, dist);
    if (alpha < 0.01) discard;
    outColor = vec4(fragColor.rgb, fragColor.a * alpha);
}