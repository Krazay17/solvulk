#include "sol_core.h"

#include "vital/vital.h"

void Sol_View_Healthbar(World *world)
{
    WAdd3d(world) = Sol_View_Healthbar_Draw;
}

void Sol_View_Healthbar_Draw(World *world, double dt, double time)
{
    int required = HAS_VITAL;
    int count    = world->activeCount;
    for (int i = 0; i < count; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;
        SolXform xform = Sol_Xform_GetDrawXform(world, id);
        xform.pos.y += Sol_Physx_GetDims(world, id).y - 1.0f;

        u32   health    = Sol_Vital_GetHealth(world, id);
        u32 maxHealth = Sol_Vital_GetMaxHealth(world, id);
        float fill      = maxHealth > 0 ? (float)health / (float)maxHealth : 0.0f;

        Sol_Render_PushBillboard((BillboardDesc){
            .kind   = BILLBOARD_HEALTHBAR,
            .pos    = (vec4s){{xform.pos.x, xform.pos.y, xform.pos.z, 1.0f}},
            .color  = (vec4s){{0.1f, 0.85f, 0.2f, 1.0f}}, // healthy green
            .params = (vec4s){{fill, 0.015f, 0.0f, 0.0f}},
        });
    }
}