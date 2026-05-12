#include "sol_core.h"

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
    u32 idx = world->draw2dCount++;
    world->draw2dSystems[idx] = Sol_Crosshair_Draw;
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
    float    fdt  = (float)dt;
    SolLook *look = Sol_Input_GetLook();

    // Can shake here?
    float changeDist = (float)Sol_Input_GetMouse().wheelV * 0.01f;
    if (Sol_Input_GetMouse().wheelV)
        camera_arm.distance -= changeDist;

    memcpy(sol_camera.position, camera_arm.arm.raw, sizeof(vec3));
    memcpy(sol_camera.target, vecAdd(camera_arm.arm, look->lookdir).raw, sizeof(vec3));

    Sol_Render_Camera_Update(&sol_camera);
}

// void Sol_Camera_Tick(World *world, double dt, double time, float alpha)
// {
//     return;
//     int id = world->playerID;
//     if (id < 0)
//         return;
//     float           fdt        = (float)dt;
//     CompXform      *xform      = &world->xforms[id];
//     CompBody       *body       = &world->bodies[id];
//     CompController *controller = &world->controllers[id];

//     vec3s lookdir      = controller->lookdir;
//     vec3s head         = xform->drawPos;
//     vec3s anchor       = xform->drawPos;
//     vec3s invDirection = glms_vec3_scale(lookdir, -1.0f);
//     float factor       = 1.0f - expf(-lerp_speed * fdt);

//     if (body)
//     {
//         anchor.y += body->dims.y * 0.4f;
//         head.y += body->dims.y * 0.4f;
//     }

//     if (!initialized)
//     {
//         arm         = anchor;
//         initialized = true;
//     }
//     vec3s camPos;
//     vec3s target;
//     distance -= Sol_Input_GetMouse().wheelV * 0.01f;
//     if (distance <= 4.4f)
//     {
//         distance = 4.4f;
//         camPos = arm = head;
//     }
//     else
//     {
//         vec3s offsetvec = glms_vec3_cross(lookdir, WORLD_UP);

//         SolRayResult anchortrace =
//             Raycast_Static_Grid_Walk(world, (SolRay){.pos = anchor, .dir = offsetvec, .dist = offset + wallbuffer});
//         float targetOffset = anchortrace.dist - wallbuffer;
//         currentOffset      = Sol_Lerp(currentOffset, targetOffset, factor * currentOffset / targetOffset);
//         anchor             = vecAdd(anchor, vecSca(offsetvec, currentOffset));

//         arm = glms_vec3_lerp(arm, anchor, factor);

//         SolRayResult camDistTrace =
//             Raycast_Static_Grid_Walk(world, (SolRay){.pos = arm, .dir = invDirection, .dist = distance +
//             wallbuffer});
//         float targetDist = camDistTrace.dist - wallbuffer;
//         currentDistance  = Sol_Lerp(currentDistance, targetDist, factor * currentDistance / targetDist);
//         if (currentDistance < 0)
//             currentDistance = 0;
//         camPos = glms_vec3_add(arm, glms_vec3_scale(invDirection, currentDistance));
//     }
//     target = glms_vec3_add(camPos, lookdir);

//     Sol_Render_Camera_Update(camPos.raw, target.raw);

//     controller->aimHitEnt = -1;
//     SolRayResult aimTrace = Sol_Raycast(world, (SolRay){
//                                                    .pos       = camPos,
//                                                    .ignoreEnt = id,
//                                                    .mask      = 0b01,
//                                                    .dir       = lookdir,
//                                                    .dist      = 100.f,
//                                                });
//     vec3s        dir      = glms_vec3_normalize(glms_vec3_sub(aimTrace.pos, head));
//     controller->aimdir    = vecDot(dir, lookdir) > 0.6f ? dir : lookdir;
//     controller->aimHitEnt = aimTrace.entId;
//     controller->aimpos    = head;

//     // Sol_Debug_Add("CamX", camPos.x);
//     // Sol_Debug_Add("CamY", camPos.y);
//     // Sol_Debug_Add("CamZ", camPos.z);
// }

void Sol_Crosshair_Draw(World *world, double dt, double time)
{
    if (world->playerID < 0)
        return;
    int width  = Sol_GetState()->windowWidth / 2;
    int height = Sol_GetState()->windowHeight / 2;
    Render_Draw_Rectangle(
        (vec4s){
            .x = width,
            .y = height,
            .z = 3,
            .w = 12,
        },
        (vec4s){255, 0, 0, 255}, 0);
    Render_Draw_Rectangle(
        (vec4s){
            .x = width,
            .y = height,
            .z = 12,
            .w = 3,
        },
        (vec4s){255, 0, 0, 255}, 0);
}

SolCameraArm *Sol_Cam_GetArm()
{
    return &camera_arm;
}