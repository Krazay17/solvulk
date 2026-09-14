#include "world.h"
#include "sol_math.h"

void Projectile_Step(World *world)
{
    float fdt = world->timestep;

    SparseSet_ScProjectile *set = Sol_Comp_Set(world, ScProjectile);
    for (int i = 0; i < set->cnt; i++)
    {
        int id                   = set->dense[i];
        ScProjectile *projectile = &set->data[i];
        ScBody3 *body3           = Sol_Comp_Get(world, id, ScBody3);
        ScOwner *owner           = Sol_Comp_Get(world, id, ScOwner);
        Xform xform              = Xform_Get(world, id);

        vec3s vel   = body3->vel;
        float speed = glms_vec3_norm(vel);
        vec3s dir   = vecSca(vel, speed > 0 ? 1.0f / speed : 1.0f);
        speed *= fdt;
        SolRay ray = {
            .start = vecAdd(xform.pos, vecSca(dir, speed)),
            .dir   = dir,
            .dist  = speed,
            .mask = COLLAYER_ALL,
            .ignoreEnt = id,
        };

        SolHit hit = {
            .entA   = id,
            .damage = 10.0f,
            .vel    = vel,
        };

        float closest = 1e9f;
        int best      = 0;
        SolRayResult results[16];
        int hits = Sol_SpherecastD(world, ray, results, 16, body3->dims.x, 0.2f);
        for (int j = 0; j < hits; j++)
        {
            SolRayResult result = results[j];
            if (result.entId == owner->ownerId)
                continue;
            vec3s hit_pos = Sol_AddScaledDir(ray.start, ray.dir, result.t);
            float d2 = glms_vec3_distance2(xform.pos, hit_pos);
            if (d2 < closest)
            {
                closest    = d2;
                best       = result.entId;
                hit.normal = result.norm;
                hit.pos    = hit_pos;
            }
        }
        if (best)
        {
            hit.entB = best;
            Sol_Combat_Hit(world, hit.entB, hit);
            Sol_Destroy_Ent(world, id);
        }
    }
}