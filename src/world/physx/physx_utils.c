#include "physx_i.h"
#include "sol_math.h"
#include "sol_core.h"
#include "world.h"
#include "profiler.h"
#include "xform/s_xform.h"

void Closest_Points_Segment_Segment(vec3s p1, vec3s q1, // segment A: p1 → q1
                                    vec3s p2, vec3s q2, // segment B: p2 → q2
                                    vec3s *outA, vec3s *outB)
{
    vec3s d1 = glms_vec3_sub(q1, p1); // segment A direction
    vec3s d2 = glms_vec3_sub(q2, p2); // segment B direction
    vec3s r  = glms_vec3_sub(p1, p2);

    float a = glms_vec3_dot(d1, d1); // squared length of segment A
    float e = glms_vec3_dot(d2, d2); // squared length of segment B
    float f = glms_vec3_dot(d2, r);

    float s, t;
    // Both segments degenerate to points?
    if (a <= FLOATING_EPSILON && e <= FLOATING_EPSILON)
    {
        *outA = p1;
        *outB = p2;
        return;
    }

    if (a <= FLOATING_EPSILON)
    {
        // Segment A is a point
        s = 0.0f;
        t = f / e;
        t = fmaxf(0.0f, fminf(1.0f, t));
    }
    else
    {
        float c = glms_vec3_dot(d1, r);

        if (e <= FLOATING_EPSILON)
        {
            // Segment B is a point
            t = 0.0f;
            s = fmaxf(0.0f, fminf(1.0f, -c / a));
        }
        else
        {
            // General case
            float b     = glms_vec3_dot(d1, d2);
            float denom = a * e - b * b;

            // Segments not parallel
            if (denom != 0.0f)
            {
                s = (b * f - c * e) / denom;
                s = fmaxf(0.0f, fminf(1.0f, s));
            }
            else
            {
                // Parallel — pick s = 0 arbitrarily
                s = 0.0f;
            }

            t = (b * s + f) / e;

            // Clamp t and recompute s if needed
            if (t < 0.0f)
            {
                t = 0.0f;
                s = fmaxf(0.0f, fminf(1.0f, -c / a));
            }
            else if (t > 1.0f)
            {
                t = 1.0f;
                s = fmaxf(0.0f, fminf(1.0f, (b - c) / a));
            }
        }
    }

    *outA = glms_vec3_add(p1, glms_vec3_scale(d1, s));
    *outB = glms_vec3_add(p2, glms_vec3_scale(d2, t));
}

void Physx_ParseModel(World *world, int id, PhysxGroup *group)
{
    CompXform *xform         = &world->xforms[id];
    u32        handle        = Sol_Model_GetModelId(world, id);
    u32        modelTriCount = Sol_Model_GetTriCount(handle);
    u32        oldCount      = group->triCount;
    u32        newCount      = oldCount + modelTriCount;

    if (newCount > group->capacity)
    {
        group->capacity = newCount * 2;
        group->tris     = realloc(group->tris, sizeof(SolTri) * group->capacity);
    }

    group->triCount = newCount;
    Transform_Tris_LocalToWorld(group->tris, id, oldCount, handle, xform);

    group->ents[id].triIndexStart = oldCount;
    group->ents[id].triIndexCount = modelTriCount;
}
