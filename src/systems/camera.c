#include "sol_core.h"

#include "xform/xform.h"
#include "controller/controller.h"
#include "physx/physx.h"

static bool  initialized = false;
static vec3s arm;
static float distance        = 6.5f;
static float currentDistance = 6.5f;
static float offset          = 1.3f;
static float currentOffset   = 1.3f;

static float lerp_speed = 30.0f; // Higher is faster/tighter

void Sol_Camera_Tick(World *world, double dt, double time, float alpha)
{
    int id = world->playerID;
    if (id < 0)
        return;
    float           fdt        = (float)dt;
    CompXform      *xform      = &world->xforms[id];
    CompBody       *body       = &world->bodies[id];
    CompController *controller = &world->controllers[id];

    vec3s lookdir      = controller->lookdir;
    vec3s head         = xform->drawPos;
    vec3s anchor       = xform->drawPos;
    vec3s invDirection = glms_vec3_scale(lookdir, -1.0f);
    float factor       = 1.0f - expf(-lerp_speed * fdt);

    if (body)
    {
        anchor.y += body->dims.y * 0.4f;
        head.y += body->dims.y * 0.4f;
    }

    if (!initialized)
    {
        arm         = anchor;
        initialized = true;
    }
    vec3s camPos;
    vec3s target;
    distance -= Sol_Input_GetMouse().wheelV * 0.01f;
    if (distance <= 4.4f)
    {
        distance = 4.4f;
        camPos = arm = head;
    }
    else
    {
        vec3s offsetvec = glms_vec3_cross(lookdir, WORLD_UP);

        SolRayResult anchortrace =
            Raycast_Static_Grid_Walk(world, (SolRay){.pos = anchor, .dir = offsetvec, .dist = offset + 1.0f});
        currentOffset = Sol_Lerp(currentOffset, anchortrace.dist - 1.0f, factor);
        anchor        = vecAdd(anchor, vecSca(offsetvec, currentOffset));

        arm = glms_vec3_lerp(arm, anchor, factor);

        SolRayResult camDistTrace =
            Raycast_Static_Grid_Walk(world, (SolRay){.pos = arm, .dir = invDirection, .dist = distance});
        currentDistance = Sol_Lerp(currentDistance, camDistTrace.dist - 3.0f, factor);
        if (currentDistance < 0)
            currentDistance = 0;
        camPos = glms_vec3_add(arm, glms_vec3_scale(invDirection, currentDistance));
    }
    target = glms_vec3_add(camPos, lookdir);

    Render_Camera_Update(camPos.raw, target.raw);

    controller->aimHitEnt = -1;
    SolRayResult aimTrace = Sol_Raycast(world, (SolRay){
                                                   .pos       = camPos,
                                                   .ignoreEnt = id,
                                                   .mask      = 0b01,
                                                   .dir       = lookdir,
                                                   .dist      = 100.f,
                                               });
    vec3s        dir      = glms_vec3_normalize(glms_vec3_sub(aimTrace.pos, head));
    controller->aimdir    = vecDot(dir, lookdir) > 0.6f ? dir : lookdir;
    controller->aimHitEnt = aimTrace.entId;
    controller->aimpos    = head;

    Sol_Debug_Add("CamX", camPos.x);
    Sol_Debug_Add("CamY", camPos.y);
    Sol_Debug_Add("CamZ", camPos.z);
}

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
