#version 450

layout(push_constant)uniform Push{
    mat4 ortho;
    float x,y,w,h;
    vec4 color;
}push;

layout(location=0)out vec4 fragColor;

void main()
{
    // generate quad from 6 vertices
    vec2 positions[6]=vec2[](
        vec2(push.x,push.y),
        vec2(push.x+push.w,push.y),
        vec2(push.x+push.w,push.y+push.h),
        vec2(push.x,push.y),
        vec2(push.x+push.w,push.y+push.h),
        vec2(push.x,push.y+push.h)
    );
    
    vec2 pos=positions[gl_VertexIndex];
    gl_Position=push.ortho*vec4(pos,0.,1.);
    fragColor=push.color;
}