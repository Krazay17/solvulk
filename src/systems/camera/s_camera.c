#include "sol_core.h"

#include "physx/physx_i.h"

SolCameraArm camera_arm = {
    .active     = false,
    .lerpspeed  = 20.0f,
    .distance   = 6.5f,
    .offset     = 0.5f,
    .wallbuffer = 0.5f,
};

SolCamera sol_camera = {
    .fov      = 60.0f,
    .nearClip = 0.2f,
    .farClip  = 1000.0f,
};

void Sol_Cam_Init(World *world)
{
}

SolCamera *Sol_GetCamera()
{
    return &sol_camera;
}

void Sol_Cam_Arm_Update(World *world, vec3s head, double dt)
{
    float fdt          = (float)dt;
    vec3s lookdir      = Sol_Input_GetLook()->lookdir;
    vec3s invDirection = glms_vec3_scale(lookdir, -1.0f);
    float factor       = 1.0f - expf(-camera_arm.lerpspeed * fdt);
    vec3s offsetvec    = glms_vec3_cross(lookdir, WORLD_UP);

    if (camera_arm.distance <= 0)
    {
        camera_arm.distance = 0;
        camera_arm.arm = camera_arm.anchor = head;
    }
    else
    {
        vec3s anchor = vecAdd(head, vecSca(offsetvec, camera_arm.offset));

        SolRayResult anchortrace = Raycast_Static_Grid_Walk(
            world, (SolRay){.pos = anchor, .dir = offsetvec, .dist = camera_arm.offset + camera_arm.wallbuffer});
        float targetOffset       = anchortrace.dist - camera_arm.wallbuffer;
        camera_arm.currentOffset = Sol_Lerp(camera_arm.currentOffset, targetOffset, factor);

        camera_arm.anchor =
            glms_vec3_lerp(camera_arm.anchor, vecAdd(anchor, vecSca(offsetvec, camera_arm.currentOffset)), factor);

        SolRayResult camDistTrace =
            Raycast_Static_Grid_Walk(world, (SolRay){.pos  = camera_arm.anchor,
                                                     .dir  = invDirection,
                                                     .dist = camera_arm.distance + camera_arm.wallbuffer});
        float targetDist = camDistTrace.dist - camera_arm.wallbuffer;
        if (targetDist < 0)
            targetDist = 0;
        camera_arm.currentDistance = Sol_Lerp(camera_arm.currentDistance, targetDist, factor);
        
        camera_arm.arm = glms_vec3_add(camera_arm.anchor, glms_vec3_scale(invDirection, camera_arm.currentDistance));
    }
}

void Sol_Cam_Update(double dt)
{
    // float    fdt  = (float)dt;
    SolLook *look = Sol_Input_GetLook();

    // Can shake here?
    float changeDist = (float)Sol_Input_GetMouse().wheelV * 0.01f;
    if (Sol_Input_GetMouse().wheelV)
        camera_arm.distance -= changeDist;

    memcpy(sol_camera.position, camera_arm.arm.raw, sizeof(vec3));
    memcpy(sol_camera.target, vecAdd(camera_arm.arm, look->lookdir).raw, sizeof(vec3));

    
    glm_vec3_lerp(sol_camera.up, (vec3){0.0f, 1.0f, 0.0f}, 1.0f - expf(-10.0f * (float)dt), sol_camera.up);

    glm_lookat(sol_camera.position, sol_camera.target, sol_camera.up, sol_camera.view);

    Sol_Render_Camera_Update(&sol_camera);
}

SolCameraArm *Sol_Cam_GetArm()
{
    return &camera_arm;
}