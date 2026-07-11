#version 450
layout(set=0, binding=0) uniform Ortho {
    mat4 ortho2d;
};
layout(push_constant) uniform Push {
    float x, y, w, h;
    float u, v, uw, vh;
    float r, g, b, a;
} push;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec4 fragColor;

void main()
{
    vec2 positions[6] = vec2[](
        vec2(push.x,         push.y),
        vec2(push.x + push.w, push.y),
        vec2(push.x + push.w, push.y + push.h),
        vec2(push.x,         push.y),
        vec2(push.x + push.w, push.y + push.h),
        vec2(push.x,         push.y + push.h)
    );

    vec2 uvs[6] = vec2[](
        vec2(push.u,          push.v),
        vec2(push.u + push.uw, push.v),
        vec2(push.u + push.uw, push.v + push.vh),
        vec2(push.u,          push.v),
        vec2(push.u + push.uw, push.v + push.vh),
        vec2(push.u,          push.v + push.vh)
    );

    fragUV    = uvs[gl_VertexIndex];
    fragColor = vec4(push.r, push.g, push.b, push.a);
    gl_Position = ortho2d * vec4(positions[gl_VertexIndex], 0.0, 1.0);
}