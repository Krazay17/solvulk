#include <cglm/struct.h>

#include "sol_core.h"

static bool  initialized = false;
static vec3s arm;
static float distance = 10.0f;
static float offset   = 1.3f;

void Cam_Update_3D(World *world, double dt, double time, float alpha)
{
    int id = world->playerID;
    if (id < 0)
        return;
    float           fdt        = (float)dt;
    CompXform      *xform      = &world->xforms[id];
    CompBody       *body       = &world->bodies[id];
    CompController *controller = &world->controllers[id];

    vec3s lookdir      = controller->lookdir;
    vec3s anchor       = xform->drawPos;
    vec3s invDirection = glms_vec3_scale(lookdir, -1.0f);

    if (body)
        anchor.y += body->height * 0.7f;

    if (!initialized)
    {
        arm         = anchor;
        initialized = true;
    }
    vec3s head = anchor;
    anchor     = glms_vec3_add(anchor, glms_vec3_scale(glms_vec3_cross(lookdir, WORLD_UP), offset));
    arm        = glms_vec3_lerp(arm, anchor, fdt * 60.0f);

    SolRayResult result = Raycast_Static_Grid_Walk(world, (SolRay){.pos = arm, .dir = invDirection, .dist = distance});
    vec3s        camPos = glms_vec3_add(arm, glms_vec3_scale(invDirection, result.dist - 2.0f));

    vec3s target = glms_vec3_add(camPos, lookdir);
    Render_Camera_Update(camPos.raw, target.raw);

    controller->aimHitEnt = -1;
    SolRayResult aimTrace = Sol_Raycast(world, (SolRay){.pos = camPos, .ignoreEnt = id, .dir = lookdir, .dist = 500.f});
    controller->aimdir    = glms_vec3_normalize(glms_vec3_sub(aimTrace.pos, head));
    controller->aimHitEnt = aimTrace.entId;
    controller->aimPos    = head;
}

void Crosshair_Draw(World *world, double dt, double time)
{
    int width  = Sol_GetState()->windowWidth / 2;
    int height = Sol_GetState()->windowHeight / 2;
    Sol_Draw_Rectangle(
        (SolRect){
            .x = width,
            .y = height,
            .h = 3,
            .w = 12,
        },
        (SolColor){255, 0, 0, 255}, 0);
    Sol_Draw_Rectangle(
        (SolRect){
            .x = width,
            .y = height,
            .h = 12,
            .w = 3,
        },
        (SolColor){255, 0, 0, 255}, 0);
}
