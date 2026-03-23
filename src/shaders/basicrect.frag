#version 450

layout(location=0)in vec4 fragColor;
layout(location=1)in vec2 fragPos;
layout(location=0)out vec4 outColor;

layout(push_constant)uniform Push{
    mat4 ortho;
    vec4 rect;// x, y, z, w
    vec4 color;// r, g, b, a
    vec4 extra;// thickness, 0, 0, 0
}push;

void main()
{
    float t=push.extra[0];
    bool inside=fragPos.x>t&&fragPos.x<push.rect[2]-t&&
    fragPos.y>t&&fragPos.y<push.rect[3]-t;
    
    if(push.extra[0]>0.&&inside)discard;
    
    outColor=fragColor;
}