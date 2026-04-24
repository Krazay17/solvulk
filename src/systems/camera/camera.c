#include <cglm/struct.h>

#include "sol_core.h"

static bool  initialized = false;
static vec3s arm;

void Cam_Update_3D(World *world, double dt, double time, float alpha)
{
    int id = world->playerID;
    if (id < 0)
        return;
    float           fdt        = (float)dt;
    CompXform      *xform      = &world->xforms[id];
    CompBody       *body       = &world->bodies[id];
    CompController *controller = &world->controllers[id];

    vec3s lookdir = controller->lookdir;
    vec3s anchor  = xform->drawPos;

    if (body)
        anchor.y += body->height * 0.5f;

    if (!initialized)
    {
        arm         = anchor;
        initialized = true;
    }

    arm = glms_vec3_lerp(arm, anchor, fdt * 60.0f);
    vec3s pos = glms_vec3_sub(arm, glms_vec3_scale(lookdir, 10.0f));
    Render_Camera_Update(pos.raw, arm.raw);
}
