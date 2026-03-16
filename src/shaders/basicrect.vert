#version 450

layout(push_constant)uniform Push{
    float x,y,w,h;
    vec4 color;
    float screenW,screenH;
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
    
    // convert from pixel coords to NDC (-1 to 1)
    vec2 ndc=positions[gl_VertexIndex]/vec2(push.screenW,push.screenH)*2.-1.;
    gl_Position=vec4(ndc,0.,1.);
    fragColor=push.color;
}