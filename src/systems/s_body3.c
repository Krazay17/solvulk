#include "world.h"
#include "physx.h"
#include "sol_math.h"

void Body3_Step(World *world, double dt, double time)
{
    float fdt = (float)dt;

    SparseSet_SolBody3 *set = Sol_Comp_Set(world, SolBody3);
    for (int i = 0; i < set->cnt; i++)
    {
        int       id    = set->dense[i];
        SolBody3 *body3 = &set->data[i];

        SolXform *xform = Sol_Comp_Get(world, id, SolXform);

        vec3s accel    = body3->vel.y < -100.0f ? GLMS_VEC3_ZERO : body3->gravity;
        accel          = glms_vec3_add(accel, body3->force);
        accel          = glms_vec3_add(accel, body3->impulse);
        body3->impulse = (vec3s){0};
        body3->vel     = glms_vec3_add(body3->vel, glms_vec3_scale(accel, fdt));

        SubstepData substep = Substep_Get(glms_vec3_norm(body3->vel), fmaxf(body3->dims.x, body3->dims.y) * 0.9f, fdt);
        for (int s = 0; s < substep.substeps; s++)
        {
            xform->pos = glms_vec3_add(xform->pos, glms_vec3_scale(body3->vel, substep.sub_dt));
            // Collisions_Static_Grid(world, staticGroup, body, xform, &contacts[j]);
        }
        if (xform->pos.y < 0)
        {
            xform->pos.y = 0;
            body3->vel.y *= -1;
        }
    }
}