#include "sol_core.h"

static void Step(World *world, double dt, double time);

void Sol_Body2d_Init(World *world)
{
    world->body2d   = calloc(MAX_ENTS, sizeof(CompBody2d));
    WAddStep(world) = Step;
}

void Sol_Body2d_Add(World *world, int id, CompBody2d desc)
{
    world->masks[id] |= HAS_BODY2;
    world->body2d[id] = desc;
}

static void Step(World *world, double dt, double time)
{
    float fdt    = (float)dt;
    float bottom = WINDOW_HEIGHT;
    float right  = WINDOW_WIDTH;

    int required = HAS_BODY2;
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;
        CompXform  *xform  = &world->xforms[id];
        CompBody2d *body   = &world->body2d[id];
        vec2s       vel    = body->vel;
        vec2s       pos    = (vec2s){xform->pos.x, xform->pos.y};
        vec2s       dims   = (vec2s){body->dims.x, body->dims.y};
        vec2s       center = (vec2s){pos.x + (dims.x * 0.5f), pos.y + (dims.y * 0.5f)};
        bool        hitX   = 0;
        bool        hitY   = 0;

        if (world->masks[id] & HAS_INTERACT)
        {
            SolMouse mouse = Sol_Input_GetMouse();

            vec2s mPos = {mouse.x, mouse.y};
            if (Sol_Check_2d_Collision(mPos, (vec4s){pos.x, pos.y, dims.x, dims.y}) && mouse.buttons[SOL_MOUSE_MIDDLE])
            {
                float dist = glms_vec2_distance(mPos, center);
                vel = glms_vec2_scale(glms_vec2_normalize(glms_vec2_sub(mPos, center)), dist);
            }
        }

        vel = glms_vec2_add(vel, glms_vec2_scale(body->grav, fdt));
        pos = glms_vec2_add(pos, vel);
        if (pos.y < 0)
        {
            pos.y = 0;
            hitY  = 1;
        }
        if (pos.x < 0)
        {
            pos.x = 0;
            hitX  = 1;
        }
        if (pos.y + dims.y > bottom)
        {
            pos.y = bottom - dims.y;
            hitY  = 1;
        }
        if (pos.x + dims.x > right)
        {
            pos.x = right - dims.x;
            hitX  = 1;
        }
        if (hitX)
            vel.x *= -1.0f;
        if (hitY)
            vel.y *= -1.0f;

        body->vel  = vel;
        xform->pos = (vec3s){pos.x, pos.y, 0};
    }
}
vec2s Sol_Body2d_GetDims(World *world, int id)
{
    return world->body2d[id].dims;
}