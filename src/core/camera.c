#include "sol_core.h"
#include "sol_engine.h"
#include "types.h"
#include "input.h"
#include "world.h"
#include "sol_math.h"
#include "xform/s_xform.h"
#include "physx/s_body.h"
#include "movement/s_movement.h"
#include "render/render.h"

#define CAMERA_LERP_SPEED 10.0f

typedef struct SolCamera
{
    mat4s proj;
    mat4s view;
    mat4s viewProj;
    vec3s pos, anchor;
    vec3s target, dir;
    vec3s up;
    float fov;
    float nearClip;
    float farClip;
    float lerpspeed;
    float roll;
    float distance, currentDistance;
    float offset, currentOffset;
} SolCamera;

SolCamera solCamera = {
    .fov       = 60.0f,
    .nearClip  = 0.2f,
    .farClip   = 1000.0f,
    .lerpspeed = 10.0f,
    .distance  = 3.5f,
    .offset    = 1.0f,
};

void Sol_Cam_Update(double dt)
{
    float  fdt   = (float)dt;
    World *world = solEngine.activeWorld;

    vec3s lookdir   = Sol_Input_GetLookDir();
    vec3s invDir    = glms_vec3_scale(lookdir, -1.0f);
    vec3s offsetvec = glms_vec3_cross(lookdir, WORLD_UP);

    vec3s head = Sol_Xform_GetDrawXform(world, 1).pos;
    head.y += Sol_Physx_GetHeight(world, 1) * 0.5f;
    if (solCamera.distance <= 0)
    {
        solCamera.distance = 0;
        solCamera.anchor   = head;
    }
    else
    {
        SolRayResult anchortrace =
            Sol_Raycast(world, (SolRay){.pos = head, .dir = offsetvec, .dist = solCamera.offset * 2.0f});
        solCamera.currentOffset = anchortrace.dist - solCamera.offset;
        vec3s offsetPos         = vecSca(offsetvec, solCamera.currentOffset);
        vec3s finalOffsetPos    = vecAdd(head, offsetPos);
        float offsetDistance    = glms_vec3_distance2(solCamera.anchor, finalOffsetPos);
        float factor            = 1.0f - expf(-(solCamera.lerpspeed + offsetDistance) * fdt);
        solCamera.anchor        = glms_vec3_lerp(solCamera.anchor, finalOffsetPos, factor);

        SolRayResult camDistTrace =
            Sol_Raycast(world, (SolRay){.pos = solCamera.anchor, .dir = invDir, .dist = solCamera.distance * 1.2f});
        float targetDist = camDistTrace.dist * 0.8f;
        if (targetDist < 0)
            targetDist = 0;
        solCamera.currentDistance = Sol_Math_Lerp(solCamera.currentDistance, targetDist, factor);
    }

    solCamera.pos = glms_vec3_add(solCamera.anchor, glms_vec3_scale(invDir, solCamera.currentDistance));

    float changeDist = (float)Sol_Input_GetMouse().wheelV * 0.01f;
    if (Sol_Input_GetMouse().wheelV)
        solCamera.distance -= changeDist;

    solCamera.target = vecAdd(solCamera.pos, lookdir);
    solCamera.dir    = glms_vec3_normalize(glms_vec3_sub(solCamera.target, solCamera.pos));

    // === Wallrun tilt ===
    float targetRoll = 0.0f;
    if (world->masks[1] & BITC(HAS_MOVEMENT))
    {
        CompMovement *m     = &world->movements[1];
        CompXform    *xform = &world->xforms[1];
        if (m->state == MOVE_WALLRUN)
        {
            vec3s dir   = vecSub(m->lastTouch, xform->pos);
            dir         = vecNorm(dir);
            vec3s right = vecCrs(solCamera.dir, WORLD_UP);
            float dot   = vecDot(right, dir);
            targetRoll  = -dot * 15.0f * (3.14159f / 180.0f);
        }
    }
    float rollFactor = 1.0f - expf(-CAMERA_LERP_SPEED * fdt); // slower lerp for cinematic feel
    solCamera.roll   = Sol_Math_Lerp(solCamera.roll, targetRoll, rollFactor);
    solCamera.up     = glms_vec3_rotate(WORLD_UP, solCamera.roll, solCamera.dir); // rotate up around forward

    solCamera.view = glms_lookat(solCamera.pos, solCamera.target, solCamera.up);

    solCamera.proj =
        glms_perspective(glm_rad(solCamera.fov), Sol_Render_GetAspect(), solCamera.nearClip, solCamera.farClip);
    solCamera.proj.raw[1][1] *= -1.0f;

    solCamera.viewProj = glms_mat4_mul(solCamera.proj, solCamera.view);

    SceneUBO *ubo       = Sol_Render_GetNext_Scene();
    ubo->view           = solCamera.view;
    ubo->proj           = solCamera.proj;
    ubo->viewProjection = solCamera.viewProj;
    ubo->cameraPos      = (vec4s){solCamera.pos.x, solCamera.pos.y, solCamera.pos.z, 1.0f};
    ubo->sun            = (vec4s){0.0f, 1.0f, 0.4f, 0.3f};
    ubo->aspect         = solState.aspectRatio;
}

vec3s Sol_Cam_GetPos()
{
    return solCamera.pos;
}
mat4s Sol_Cam_GetViewProj()
{
    return solCamera.viewProj;
}
vec3s Sol_Cam_GetRight()
{
    return glms_vec3_cross(solCamera.dir, WORLD_UP);
}
vec3s Sol_Cam_GetFwd()
{
    return solCamera.dir;
}