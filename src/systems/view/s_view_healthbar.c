#include "sol_core.h"

#include "vital/vital.h"

#define HEALTHBAR_HIDE_DURATION 5.0f

void Sol_View_Healthbar(World *world)
{
    WAdd3d(world) = Sol_View_Healthbar_Draw;
}

void Sol_View_Healthbar_Draw(World *world, double dt, double time)
{
    int required = HAS_ACTIVE | HAS_VITAL;
    int count    = world->activeCount;
    for (int i = 0; i < count; i++)
    {
        int id = world->activeEntities[i];
        if (id == 1)
            continue;
        if (Sol_Vital_GetDead(world, id) || (world->masks[id] & required) != required)
            continue;
        if (time - Sol_Vital_GetLastHitTime(world, id) > HEALTHBAR_HIDE_DURATION)
            continue;
        SolXform xform = Sol_Xform_GetDrawXform(world, id);
        xform.pos.y += Sol_Physx_GetDims(world, id).y * 0.77f;

        float health    = Sol_Vital_GetHealth(world, id);
        float maxHealth = Sol_Vital_GetMaxHealth(world, id);
        float fill      = maxHealth > 0 ? health / maxHealth : 0.0f;

        QuadSSBO *ssbo = Sol_Render_GetNext_Quad(QUADKIND_HEALTH);
        if (!ssbo)
            continue;

        float healthbarWidthScale = 1.0f;
        ssbo->pos                 = (vec4s){xform.pos.x, xform.pos.y, xform.pos.z, healthbarWidthScale};
        ssbo->rot                 = GLMS_VEC4_ZERO;
        ssbo->color               = (vec4s){0.1f, 0.85f, 0.2f, 1.0f};
        ssbo->uv                  = (vec4s){0, 0, 1, 1};
        // Shader Parameters Allocation (x = fill percentage, y = border outline scale)
        ssbo->extra = (vec4s){fill, 0.015f, 0, 0};
        // Explicit Subsystem Binding Types
        ssbo->type      = QUADTYPE_FACECAM;
        ssbo->flags     = 0;
        ssbo->textureId = 0;
    }
}