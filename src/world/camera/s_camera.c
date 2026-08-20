#include "s_camera.h"
#include "world.h"
#include "sol_math.h"

#include "xform/s_xform.h"
#include "controller/s_controller.h"
#include "physx/s_body.h"
#include "movement/s_movement.h"
#include "render/render.h"

const static CompCam cam_kinds[CAMKIND_COUNT] = {
    [CAMKIND_3D] =
        {
            .fov       = 60.0f,
            .lerpspeed = 20.0f,
            .distance  = 2.66f,
            .offset    = 0.66f,
        },
    [CAMKIND_NOARM] =
        {
            .fov       = 60.0f,
            .lerpspeed = 20.0f,
        },

};

typedef struct
{
    int      cnt, cap;
    int     *sparse, *dense;
    CompCam *cams;
    int      active;
} WorldCams;

static Cam_Tick(World *world, double dt, double time)
{
    float      fdt = (float)dt;
    WorldCams *wc  = world->dense_components[WORLD_SYS_CAM];
    for (int i = 0; i < wc->cnt; i++)
    {
        int        id    = wc->dense[i];
        CompCam   *cam   = &wc->cams[i];
        CompXform *xform = &world->xforms[id];

        vec3s head = xform->drawPos;
        head.y += Sol_Physx_GetHeight(world, id) * 0.5f;

        vec3s lookdir = (vec3s){0, 0, -1};
        if (WHasB(world, id, HAS_CONTROLLER))
        {
            lookdir = Sol_Controller_Get(world, id)->lookdir;
        }
        vec3s invDir    = glms_vec3_scale(lookdir, -1.0f);
        vec3s offsetvec = glms_vec3_cross(lookdir, WORLD_UP);

        if (cam->distance <= 0)
        {
            cam->distance = 0;
            cam->anchor   = head;
        }
        else
        {
            SolRayResult anchortrace =
                Sol_Raycast(world, (SolRay){.pos = head, .dir = offsetvec, .dist = cam->offset * 2.0f});
            cam->currentOffset   = anchortrace.dist - cam->offset;
            vec3s offsetPos      = vecSca(offsetvec, cam->currentOffset);
            vec3s finalOffsetPos = vecAdd(head, offsetPos);
            float offsetDistance = glms_vec3_distance2(cam->anchor, finalOffsetPos);
            float factor         = 1.0f - expf(-(cam->lerpspeed + offsetDistance) * fdt);
            cam->anchor          = glms_vec3_lerp(cam->anchor, finalOffsetPos, factor);

            SolRayResult camDistTrace =
                Sol_Raycast(world, (SolRay){.pos = cam->anchor, .dir = invDir, .dist = cam->distance * 1.2f});
            float targetDist = camDistTrace.dist * 0.8f;
            if (targetDist < 0)
                targetDist = 0;
            cam->currentDistance = Sol_Math_Lerp(cam->currentDistance, targetDist, factor);
        }

        cam->pos    = glms_vec3_add(cam->anchor, glms_vec3_scale(invDir, cam->currentDistance));
        cam->target = vecAdd(cam->pos, lookdir);
        cam->dir    = glms_vec3_normalize(glms_vec3_sub(cam->target, cam->pos));

        // === Wallrun tilt ===
        float targetRoll = 0.0f;
        if (world->masks[id] & BITC(HAS_MOVEMENT))
        {
            CompMovement *m = &world->movements[id];
            if (m->state == MOVE_WALLRUN)
            {
                vec3s dir   = vecSub(m->lastTouch, xform->pos);
                dir         = vecNorm(dir);
                vec3s right = vecCrs(cam->dir, WORLD_UP);
                float dot   = vecDot(right, dir);
                targetRoll  = -dot * 15.0f * (3.14159f / 180.0f);
            }
        }
        float rollFactor = 1.0f - expf(-CAMERA_LERP_SPEED * fdt); // slower lerp for cinematic feel
        cam->roll        = Sol_Math_Lerp(cam->roll, targetRoll, rollFactor);
        cam->up          = glms_vec3_rotate(WORLD_UP, cam->roll, cam->dir); // rotate up around forward
        if (id == wc->active)
        {
            solCamera.pos    = cam->pos;
            solCamera.dir    = cam->dir;
            solCamera.roll   = cam->roll;
            solCamera.target = cam->target;
            solCamera.up     = cam->up;
        }
    }
}

void Sol_Cam_Init(World *world)
{
    WorldCams *wc                          = malloc(sizeof(WorldCams));
    world->dense_components[WORLD_SYS_CAM] = wc;
    wc->cap                                = 1;
    wc->cnt                                = 0;
    wc->sparse                             = malloc(MAX_ENTS * sizeof(int));
    wc->dense                              = malloc(wc->cap * sizeof(int));
    wc->cams                               = malloc(wc->cap * sizeof(CompCam));
    memset(wc->sparse, -1, MAX_ENTS * sizeof(int));

    WAddTick(world) = Cam_Tick;
}

CompCam *Sol_Cam_Add(World *world, int id, CamKind kind, bool active)
{
    WorldCams *wc = world->dense_components[WORLD_SYS_CAM];
    if (active)
        wc->active = id;

    if (wc->sparse[id] != -1)
        return &wc->cams[wc->sparse[id]];
    if (wc->cnt >= wc->cap)
    {
        wc->cap *= 2;
        wc->dense = realloc(wc->dense, wc->cap * sizeof(int));
        wc->cams  = realloc(wc->cams, wc->cap * sizeof(CompCam));
    }
    int idx        = wc->cnt++;
    wc->sparse[id] = idx;
    wc->dense[idx] = id;
    CompCam *cam   = &wc->cams[idx];
    *cam           = cam_kinds[kind];
    WAddComp(world, id, HAS_CAMERA);
    return cam;
}

CompCam *Sol_Cam_Get(World *world, int id)
{
    WorldCams *wc = world->dense_components[WORLD_SYS_CAM];
    return &wc->cams[wc->sparse[id]];
}

void Sol_Cam_Remove(World *world, int id)
{
    WorldCams *wc = world->dense_components[WORLD_SYS_CAM];
    for (int i = 0; i < wc->cnt; i++)
    {
        if (wc->dense[i] == id)
        {
            wc->sparse[id] = -1;
            wc->cams[i]    = wc->cams[--wc->cnt];
        }
    }
}

void Sol_Cam_AdjustDistance(World *world, int id, float delta)
{
    WorldCams *wc  = world->dense_components[WORLD_SYS_CAM];
    CompCam   *cam = &wc->cams[wc->sparse[id]];
    cam->distance += delta;
}

void Sol_Cam_Activate(World *world, int id)
{
    WorldCams *wc = world->dense_components[WORLD_SYS_CAM];
    if (wc->sparse[id] != -1)
    {
        world->active_cam = &wc->cams[wc->sparse[id]];
    }
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
