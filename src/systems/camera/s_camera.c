#include "sol_core.h"

SolCamera sol_camera = {
    .fov       = 60.0f,
    .nearClip  = 0.2f,
    .farClip   = 1000.0f,
    .lerpspeed = 20.0f,
    .distance  = 6.5f,
    .offset    = 1.0f,
};

void Sol_Cam_Update(double dt)
{
    float    fdt   = (float)dt;
    World   *world = Sol_State_GetActiveWorld();
    SolLook *look  = Sol_Input_GetLook();

    vec3s lookdir   = look->lookdir;
    vec3s invDir    = glms_vec3_scale(lookdir, -1.0f);
    vec3s offsetvec = glms_vec3_cross(lookdir, WORLD_UP);
    float factor    = 1.0f - expf(-sol_camera.lerpspeed * fdt);

    vec3s head = Sol_Xform_GetDrawXform(world, 1).pos;
    head.y += Sol_Physx_GetHeight(world, 1) * 0.5f;
    if (sol_camera.distance <= 0)
    {
        sol_camera.distance = 0;
        sol_camera.anchor   = head;
    }
    else
    {
        SolRayResult anchortrace =
            Sol_Raycast(world, (SolRay){.pos = head, .dir = offsetvec, .dist = sol_camera.offset * 2.0f});
        sol_camera.currentOffset = anchortrace.dist - sol_camera.offset;
        sol_camera.anchor =
            glms_vec3_lerp(sol_camera.anchor, vecAdd(head, vecSca(offsetvec, sol_camera.currentOffset)), factor);

        SolRayResult camDistTrace =
            Sol_Raycast(world, (SolRay){.pos = sol_camera.anchor, .dir = invDir, .dist = sol_camera.distance * 1.2f});
        float targetDist = camDistTrace.dist * 0.8f;
        if (targetDist < 0)
            targetDist = 0;
        sol_camera.currentDistance = Sol_Math_Lerp(sol_camera.currentDistance, targetDist, factor);

        sol_camera.pos    = glms_vec3_add(sol_camera.anchor, glms_vec3_scale(invDir, sol_camera.currentDistance));
    }

    float changeDist = (float)Sol_Input_GetMouse().wheelV * 0.01f;
    if (Sol_Input_GetMouse().wheelV)
        sol_camera.distance -= changeDist;

    sol_camera.target = vecAdd(sol_camera.pos, look->lookdir);

    sol_camera.dir = glms_vec3_normalize(glms_vec3_sub(sol_camera.pos, sol_camera.target));

    // === Wallrun tilt ===
    float targetRoll = 0.0f;
    if (world->masks[1] & HAS_MOVEMENT)
    {
        CompMovement *m = &world->movements[1];
        if (m->moveState == MOVE_WALLRUN)
        {
            targetRoll = (float)m->wallDot * 15.0f * (3.14159f / 180.0f);
        }
    }
    float rollFactor = 1.0f - expf(-8.0f * fdt); // slower lerp for cinematic feel
    sol_camera.roll  = Sol_Math_Lerp(sol_camera.roll, targetRoll, rollFactor);
    sol_camera.up  = glms_vec3_rotate(WORLD_UP, sol_camera.roll, sol_camera.dir); // rotate up around forward

    sol_camera.view = glms_lookat(sol_camera.pos, sol_camera.target, sol_camera.up);
    Sol_Render_Camera_Update(&sol_camera);
}

SolCamera *Sol_GetCamera()
{
    return &sol_camera;
}

vec3s Sol_Cam_GetRight()
{
    return glms_vec3_cross(sol_camera.dir, WORLD_UP);
}
vec3s Sol_Cam_GetFwd()
{
    return sol_camera.dir;
}