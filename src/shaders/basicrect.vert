#version 450

layout(push_constant)uniform Push{
    mat4 ortho;
    vec4 rect;// x, y, z, w
    vec4 color;// r, g, b, a
    vec4 extra;// thickness, 0, 0, 0
}push;

layout(location=0)out vec4 fragColor;
layout(location=1)out vec2 fragPos;// pixel position within the quad

void main()
{
    vec2 positions[6]=vec2[](
        vec2(push.rect[0],push.rect[1]),
        vec2(push.rect[0]+push.rect[2],push.rect[1]),
        vec2(push.rect[0]+push.rect[2],push.rect[1]+push.rect[3]),
        vec2(push.rect[0],push.rect[1]),
        vec2(push.rect[0]+push.rect[2],push.rect[1]+push.rect[3]),
        vec2(push.rect[0],push.rect[1]+push.rect[3])
    );
    
    vec2 pos=positions[gl_VertexIndex];
    fragPos=pos-vec2(push.rect[0],push.rect[1]);// local position 0..w, 0..h
    fragColor=vec4(push.color[0],push.color[1],push.color[2],push.color[3]);
    gl_Position=push.ortho*vec4(pos,0.,1.);
}