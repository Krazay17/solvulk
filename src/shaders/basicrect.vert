#version 450

layout(push_constant)uniform Push{
    mat4 ortho;
    float x,y,w,h;
    float r,g,b,a;
    float thickness;// add this
}push;

layout(location=0)out vec4 fragColor;
layout(location=1)out vec2 fragPos;// pixel position within the quad

void main()
{
    vec2 positions[6]=vec2[](
        vec2(push.x,push.y),
        vec2(push.x+push.w,push.y),
        vec2(push.x+push.w,push.y+push.h),
        vec2(push.x,push.y),
        vec2(push.x+push.w,push.y+push.h),
        vec2(push.x,push.y+push.h)
    );
    
    vec2 pos=positions[gl_VertexIndex];
    fragPos=pos-vec2(push.x,push.y);// local position 0..w, 0..h
    fragColor=vec4(push.r,push.g,push.b,push.a);
    gl_Position=push.ortho*vec4(pos,0.,1.);
}