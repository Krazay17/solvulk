#version 450

layout(location=0)in vec4 fragColor;
layout(location=1)in vec2 fragPos;
layout(location=0)out vec4 outColor;

layout(push_constant)uniform Push{
    vec4 rect;
    vec4 color;
    vec4 extra;// thickness, fill, 0, 0
}push;

void main()
{
    float t = push.extra[0];
    float progress = push.extra[1];

    // 1. Calculate the absolute X coordinate where the progress bar ends
    float progressCutoff = push.rect[2] * progress;

    // 2. Discard the pixel if it's past the progress cutoff from left-to-right
    if (fragPos.x > progressCutoff) {
        discard;
    }

    // 3. Your existing border/thickness hollow-out logic
    bool inside = fragPos.x > t && fragPos.x < progressCutoff - t &&
                  fragPos.y > t && fragPos.y < push.rect[3] - t;
    
    if (push.extra[0] > 0.0 && inside) {
        discard;
    }
    
    outColor = fragColor;
}