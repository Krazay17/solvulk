#version 450

layout(location=0)out vec4 fragColor;
layout(location=1)out vec2 fragPos;// pixel position within the quad

struct RectData{
    mat4 ortho;
    vec4 rect;// x, y, z, w
    vec4 color;// r, g, b, a
    vec4 extra;// thickness, 0, 0, 0
};

layout(std430,binding=0)readonly buffer RectBuffer{
    RectData instances[];
};

void main()
{
    RectData data=instances[gl_InstanceIndex];
    
    vec2 positions[6]=vec2[](
        vec2(data.rect[0],data.rect[1]),
        vec2(data.rect[0]+data.rect[2],data.rect[1]),
        vec2(data.rect[0]+data.rect[2],data.rect[1]+data.rect[3]),
        vec2(data.rect[0],data.rect[1]),
        vec2(data.rect[0]+data.rect[2],data.rect[1]+data.rect[3]),
        vec2(data.rect[0],data.rect[1]+data.rect[3])
    );
    
    vec2 pos=positions[gl_VertexIndex];
    fragPos=pos-vec2(data.rect.x,data.rect.y);// local position 0..w, 0..h
    fragColor=vec4(data.color.r,data.color.g,data.color.b,data.color.a);
    gl_Position=data.ortho*vec4(pos,0.,1.);
}