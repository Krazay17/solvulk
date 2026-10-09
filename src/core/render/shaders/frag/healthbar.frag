#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;
layout(location = 2) in vec4 fragExtra; // x = health fill, y = energy fill
layout(location = 3) flat in uint fragTextureId;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform Game {
    double gameTime;
} game;

void main()
{
    float healthFill = clamp(fragExtra.x, 0.0, 1.0);
    float energyFill = clamp(fragExtra.y, 0.0, 1.0);
    float t = float(game.gameTime);

    const float ENERGY_HEIGHT = 1.0 / 3.0;

    // V is flipped in the vertex shader:
    // fragUV.y = 1 is the bottom, fragUV.y = 0 is the top.
    // Energy occupies the bottom third.
    if (fragUV.y > 1.0 - ENERGY_HEIGHT)
    {
        float energyX = fragUV.x;
        float energyY = (fragUV.y - (1.0 - ENERGY_HEIGHT))
                      / ENERGY_HEIGHT;

        float isFilled = step(energyX, energyFill);

        vec3 yellow = vec3(1.0, 0.85, 0.05);
        vec3 emptyColor = vec3(0.12, 0.10, 0.025);

        float edgeGlow = 1.0 - smoothstep(
            0.0, 0.04, abs(energyX - energyFill)
        );

        vec3 filledColor = yellow * (0.9 + 0.1 * edgeGlow);

        float borderX = smoothstep(0.0, 0.025, energyX)
                      * (1.0 - smoothstep(0.975, 1.0, energyX));

        float borderY = smoothstep(0.0, 0.12, energyY)
                      * (1.0 - smoothstep(0.88, 1.0, energyY));

        float border = borderX * borderY;

        vec3 color = mix(emptyColor, filledColor, isFilled);
        color *= mix(0.65, 1.0, border);

        outColor = vec4(color, fragColor.a);
        return;
    }

    // Health occupies the upper two-thirds.
    // UV y decreases toward the top, so local Y is inverted here.
    float healthY = 1.0 - fragUV.y / (1.0 - ENERGY_HEIGHT);

    float wave1 = sin(fragUV.x * 8.0 - t * 2.5 + healthY * 2.0);
    float wave2 = cos(fragUV.x * 16.0 + t * 1.8 - healthY * 4.0);

    float energy = pow(
        (wave1 + wave2 * 0.5) * 0.25 + 0.5,
        1.5
    );

    vec3 filledColor = mix(
        fragColor.rgb * 0.85,
        fragColor.rgb * 1.45,
        energy
    );

    vec3 emptyColor = fragColor.rgb * 0.15;

    float isFilled = step(fragUV.x, healthFill);
    vec3 baseColor = mix(emptyColor, filledColor, isFilled);

    // Health local Y runs from bottom (0) to top (1).
    vec2 healthUV = vec2(fragUV.x, healthY);

    float borderX = smoothstep(0.0, 0.04, healthUV.x)
                  * (1.0 - smoothstep(0.96, 1.0, healthUV.x));

    float borderY = smoothstep(0.0, 0.04, healthUV.y)
                  * (1.0 - smoothstep(0.96, 1.0, healthUV.y));

    float border = borderX * borderY;

    outColor = vec4(
        baseColor * mix(0.6, 1.0, border),
        fragColor.a
    );
}
