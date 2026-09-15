/*
 * File: s_projectile.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-14
 *
 */
#include "world.h"
#include "sol_math.h"

typedef bool isDestroyed;

static inline isDestroyed FireballHit(World *w, int a, ScProjectile *projectile, SolHit hit)
{
    if (Sol_Hitgen_Try(w, a, hit.entB, projectile->hitgen))
    {
        Sol_Combat_Hit(w, hit.entB, hit);
        ScOwner *owner = Sol_Comp_Get(w, a, ScOwner);
        int ownerId    = owner ? owner->ownerId : 0;

        SolRay ray = {
            .start = w->xform.pos[a], .ignoreEnt = a, .dir = WORLD_DOWN, .mask = COLLAYER_ALL, .radius = 3.0f};
        SolRayResult results[32];
        int hits = Sol_SphereOverlap(w, ray, results, 32);
        for (int i = 0; i < hits; i++)
        {
            int hit_id = results[i].entId;
            if (hit_id == ownerId)
                continue;
            Sol_Combat_Hit(w, hit_id,
                           (SolHit){
                               .damage     = 50.0f,
                               .pos        = Sol_AddScaledDir(ray.start, ray.dir, results[i].t),
                               .effectMask = EFFECTMASK_KNOCKUP,
                           });
        }
        Emitter emitter = emitter_kinds[EMITTERKIND_SPHERE_BURST_FRACTAL];
        emitter.p_scale = projectile->power * 2.0f;
        emitter.p_speed = projectile->power * 10.0f;
        emitter.p_lifespan = 0.5f;
        emitter.burst = 15;
        emitter.ttl = 0;
        Sol_Emitter_Push(w, hit.pos, emitter);
    }
    // if (Sol_Comp_Has(w, hit.entB, ScStage))
    // {
    //     Sol_Destroy_Ent(w, a);
    //     return true;
    // }
    return false;
}

void Projectile_Step(World *world)
{
    float fdt                   = world->timestep;
    SparseSet_ScProjectile *set = Sol_Comp_Set(world, ScProjectile);
    for (int i = set->cnt; i-- > 0;)
    {
        int id                   = set->dense[i];
        ScProjectile *projectile = &set->data[i];
        ScBody3 *body3           = Sol_Comp_Get(world, id, ScBody3);
        ScOwner *owner           = Sol_Comp_Get(world, id, ScOwner);
        int ownerId              = owner ? owner->ownerId : 0;
        Xform xform              = Xform_Get(world, id);

        vec3s vel   = body3->vel;
        float speed = glms_vec3_norm(vel);
        vec3s dir   = (speed > 0.001f) ? vecSca(vel, 1.0f / speed) : (vec3s){0, 0, 1};

        SolRay ray = {
            .start     = xform.pos,
            .dir       = dir,
            .dist      = speed * fdt,
            .mask      = projectile->mask,
            .ignoreEnt = id,
        };

        SolRayResult results[16];
        int hits = Sol_SpherecastD(world, ray, results, 16, body3->dims.x, 0.2f);

        if (hits == 0)
            continue;

        Hook hook = projectile->hook;
        for (int j = 0; j < hits; j++)
        {
            SolRayResult result = results[j];

            // Skip owner collision
            if (result.entId == ownerId)
                continue;
            SolHit hit = projectile->hit;
            hit.entB   = result.entId;
            hit.normal = result.norm;
            hit.pos    = Sol_AddScaledDir(ray.start, ray.dir, result.t);
            hit.vel    = vecNorm(vel);

            bool destroyed = false;
            switch (projectile->kind)
            {
            case PROJECTILEKIND_FIREBALL:
                destroyed = FireballHit(world, id, projectile, hit);
                break;
            }
            if (hook)
                hook(world, id, result.entId);
            if (destroyed)
                break;
        }
    }
}