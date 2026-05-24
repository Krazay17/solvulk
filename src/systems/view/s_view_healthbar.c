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
        xform.pos.y += Sol_Physx_GetDims(world, id).y * 0.77f;

        u32   health    = Sol_Vital_GetHealth(world, id);
        u32   maxHealth = Sol_Vital_GetMaxHealth(world, id);
        float fill      = maxHealth > 0 ? (float)health / (float)maxHealth : 0.0f;

        QuadSSBO *ssbo = Sol_Render_GetNext_Quad(QUADKIND_HEALTH);
        if (!ssbo)
            continue; // Boundary safety guard

        // 1. Position Setup (X, Y, Z, and W = Dynamic 3D scale/size width)
        float healthbarWidthScale = 1.0f; // Adjust this to make bars physically wider on screen!
        ssbo->pos[0]               = xform.pos.x;
        ssbo->pos[1]               = xform.pos.y;
        ssbo->pos[2]               = xform.pos.z;
        ssbo->pos[3]               = healthbarWidthScale;

        // 2. Clear out Rotation (Facecam pipeline handles this automatically)
        ssbo->rot[0] = 0.0f;
        ssbo->rot[1] = 0.0f;
        ssbo->rot[2] = 0.0f;
        ssbo->rot[3] = 0.0f;

        // 3. Core Color Vector Tinting (Green)
        ssbo->color[0] = 0.1f;
        ssbo->color[1] = 0.85f;
        ssbo->color[2] = 0.2f;
        ssbo->color[3] = 1.0f;

        // 4. CRITICAL FIX: Base UV Coordinates Initialization
        // Sets offset to (0,0) and width/height multiplier to (1,1) so the full area is drawn
        ssbo->uv[0] = 0.0f; // Offset X
        ssbo->uv[1] = 0.0f; // Offset Y
        ssbo->uv[2] = 1.0f; // Scale Width
        ssbo->uv[3] = 1.0f; // Scale Height

        // 5. Shader Parameters Allocation (x = fill percentage, y = border outline scale)
        ssbo->extra[0] = fill;
        ssbo->extra[1] = 0.015f;
        ssbo->extra[2] = 0.0f;
        ssbo->extra[3] = 0.0f;

        // 6. Explicit Subsystem Binding Types
        ssbo->type      = QUADTYPE_FACECAM;
        ssbo->flags     = 0;
        ssbo->textureId = 0;
    }
}