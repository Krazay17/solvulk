#include "sol_core.h"

#include "vital/vital.h"

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
        if (Sol_Vital_GetDead(world, id) || (world->masks[id] & required) != required)
            continue;
        SolXform xform = Sol_Xform_GetDrawXform(world, id);
        xform.pos.y += Sol_Physx_GetDims(world, id).y * 0.77f;

        u32   health    = Sol_Vital_GetHealth(world, id);
        u32   maxHealth = Sol_Vital_GetMaxHealth(world, id);
        float fill      = maxHealth > 0 ? (float)health / (float)maxHealth : 0.0f;

        QuadSSBO *ssbo = Sol_Render_GetNext_Quad(QUADKIND_HEALTH);
        if (!ssbo)
            continue; // Boundary safety guard

        // 1. Position Setup (X, Y, Z, and W = Dynamic 3D scale/size width)
        float healthbarWidthScale = 1.0f; // Adjust this to make bars physically wider on screen!
        ssbo->pos = (vec4s){xform.pos.x, xform.pos.y, xform.pos.z, healthbarWidthScale};

        // 2. Clear out Rotation (Facecam pipeline handles this automatically)
        ssbo->rot = GLMS_VEC4_ZERO;

        // 3. Core Color Vector Tinting (Green)
        ssbo->color = (vec4s){0.1f, 0.85f, 0.2f, 1.0f};

        // 4. CRITICAL FIX: Base UV Coordinates Initialization
        // Sets offset to (0,0) and width/height multiplier to (1,1) so the full area is drawn
        ssbo->uv = (vec4s){0,0,1,1};

        // 5. Shader Parameters Allocation (x = fill percentage, y = border outline scale)
        ssbo->extra = (vec4s){fill, 0.015f, 0,0};

        // 6. Explicit Subsystem Binding Types
        ssbo->type      = QUADTYPE_FACECAM;
        ssbo->flags     = 0;
        ssbo->textureId = 0;
    }
}