#include "world.h"
#include "sol_math.h"

void Timer_Update(World *world, double dt)
{
    float fdt = (float)dt;

    SparseSet_ScTimer *set = Sol_Comp_Set(world, ScTimer);

    int count = set->cnt;
    for (int i = count - 1; i >= 0; i--)
    {
        int id         = set->dense[i];
        ScTimer *timer = &set->data[i];
        timer->elapsed += fdt;

        float remaining = timer->duration - timer->elapsed;
        // Clamp shrink window so short durations (< 1.0s) shrink over their full lifetime
        float shrink_window = fminf(0.25f, timer->duration);

        if (timer->shrinkout && remaining <= shrink_window && remaining > 0.0f)
        {
            world->xform.sca[id] = glms_vec3_lerpc(GLMS_VEC3_ZERO, world->xform.sca[id], remaining / shrink_window);
        }

        if (timer->elapsed >= timer->duration && timer->destroy)
        {
            Sol_Destroy_Ent(world, id);
        }
    }
}