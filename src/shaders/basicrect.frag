#version 450

layout(location=0)in vec4 fragColor;
layout(location=1)in vec2 fragPos;
layout(location=0)out vec4 outColor;

layout(push_constant)uniform Push{
    mat4 ortho;
    float x,y,w,h;
    float r,g,b,a;
    float thickness;
}push;

void main()
{
    float t=push.thickness;
    bool inside=fragPos.x>t&&fragPos.x<push.w-t&&
    fragPos.y>t&&fragPos.y<push.h-t;
    
    if(push.thickness>0.&&inside)discard;
    
    outColor=fragColor;
}