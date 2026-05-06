#include "camera.h"
#include "sol_core.h"

#include "controller/controller.h"
#include "physx/physx.h"
#include "xform/xform.h"

SolCameraArm camArm = {
    .active     = false,
    .lerpspeed  = 20.0f,
    .distance   = 6.5f,
    .offset     = 0.5f,
    .wallbuffer = 0.5f,
};

Sol_Cam_Arm_Update(World *world, vec3s head, double dt)
{
    float fdt          = (float)dt;
    vec3s lookdir      = Sol_Input_GetLook()->lookdir;
    vec3s invDirection = glms_vec3_scale(lookdir, -1.0f);
    float factor       = 1.0f - expf(-camArm.lerpspeed * fdt);
    vec3s offsetvec    = glms_vec3_cross(lookdir, WORLD_UP);

    if (camArm.distance <= 0)
    {
        camArm.distance = 0;
        camArm.arm = camArm.anchor = head;
    }
    else
    {
        vec3s anchor = vecAdd(head, vecSca(offsetvec, camArm.offset));

        SolRayResult anchortrace = Raycast_Static_Grid_Walk(
            world, (SolRay){.pos = anchor, .dir = offsetvec, .dist = camArm.offset + camArm.wallbuffer});
        float targetOffset   = anchortrace.dist - camArm.wallbuffer;
        camArm.currentOffset = Sol_Lerp(camArm.currentOffset, targetOffset, factor);

        camArm.anchor = glms_vec3_lerp(camArm.anchor, vecAdd(anchor, vecSca(offsetvec, camArm.currentOffset)), factor);

        SolRayResult camDistTrace = Raycast_Static_Grid_Walk(
            world, (SolRay){.pos = camArm.anchor, .dir = invDirection, .dist = camArm.distance + camArm.wallbuffer});
        float targetDist = camDistTrace.dist - camArm.wallbuffer;
        if (targetDist < 0)
            targetDist = 0;
        camArm.currentDistance = Sol_Lerp(camArm.currentDistance, targetDist, factor);

        camArm.arm = glms_vec3_add(camArm.anchor, glms_vec3_scale(invDirection, camArm.currentDistance));
    }
}

void Sol_Cam_Update(double dt)
{
    float    fdt  = (float)dt;
    SolLook *look = Sol_Input_GetLook();

    // Can shake here?
    float changeDist = (float)Sol_Input_GetMouse().wheelV * 0.01f;
    if (Sol_Input_GetMouse().wheelV)
        camArm.distance -= changeDist;

//    camArm.pos = camArm.arm;
    Render_Camera_Update(camArm.arm.raw, vecAdd(camArm.arm, look->lookdir).raw);
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

//     Render_Camera_Update(camPos.raw, target.raw);

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
    return &camArm;
}