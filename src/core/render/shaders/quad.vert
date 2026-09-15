#version 450

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec4 fragColor;
layout(location = 2) out vec4 fragExtra;
layout(location = 3) flat out uint fragTextureId;

layout(set = 1, binding = 0) uniform Scene {
    mat4 viewProjection;
    mat4 view;
    mat4 proj;
    vec4 cameraPos;
    vec4 sun;
} scene;

struct Quad {
    vec4 pos;       // xyz = world position, w = uniform scale
    vec4 rect;      // xy = local offset, zw = dimensions
    vec4 color;
    vec4 uv;        // xy = UV offset, zw = UV scale
    vec4 rot;       // quaternion (xyzw) OR rot.x = spin angle
    vec4 extra;     // custom fragment parameters
    uint type;      // 0 = QUADTYPE_FACECAM, 1 = QUADTYPE_QUAT
    uint textureId;
    uint flags;
    uint _pad;
};

layout(set = 2, binding = 0) readonly buffer Quads {
    Quad quads[];
};

const vec2 CORNERS[6] = vec2[](
    vec2(-0.5, -0.5), vec2( 0.5, -0.5), vec2( 0.5,  0.5),
    vec2(-0.5, -0.5), vec2( 0.5,  0.5), vec2(-0.5,  0.5)
);

const uint QUADTYPE_FACECAM = 0u;
const uint QUADTYPE_QUAT    = 1u;

vec3 rotateByQuat(vec4 q, vec3 v) {
    return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}

void main() {
    Quad q = quads[gl_InstanceIndex];
    vec2 corner = CORNERS[gl_VertexIndex];

    float scale = q.pos.w;
    vec2 dims = (q.rect.z > 0.0 && q.rect.w > 0.0) ? q.rect.zw : vec2(1.0);
    vec2 localPos = q.rect.xy + (corner * dims * scale);

    vec3 worldPos;

    if (q.type == QUADTYPE_FACECAM) {
        float angle = q.rot.x;
        if (angle != 0.0) {
            float c = cos(angle), s = sin(angle);
            localPos = vec2(localPos.x * c - localPos.y * s, localPos.x * s + localPos.y * c);
        }

        vec3 right = vec3(scene.view[0][0], scene.view[1][0], scene.view[2][0]);
        vec3 up    = vec3(scene.view[0][1], scene.view[1][1], scene.view[2][1]);

        worldPos = q.pos.xyz + (right * localPos.x) + (up * localPos.y);
    } else {
        worldPos = q.pos.xyz + rotateByQuat(q.rot, vec3(localPos, 0.0));
    }

    fragUV        = q.uv.xy + (corner + 0.5) * q.uv.zw;
    fragColor     = q.color;
    fragExtra     = q.extra;
    fragTextureId = q.textureId;

    gl_Position = scene.viewProjection * vec4(worldPos, 1.0);
}