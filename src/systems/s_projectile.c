/*
 * File: s_projectile.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-14
 *
 */
#include "world.h"
#include "sol_math.h"
#include "sol_core.h"

typedef bool isDestroyed;

static inline isDestroyed FireballHit(World *w, int a, ScProjectile *projectile, SolHit hit)
{
    float explode_radius = projectile->radius * 3.0f;
    ScOwner *owner       = Sol_Comp_Get(w, a, ScOwner);
    int ownerId          = owner ? owner->ownerId : 0;
    vec3s pos            = w->xform.pos[a];
    Sol_Combat_Hit(w, hit.entB, hit);

    SolRay ray = {.start = pos, .ignoreEnt = a, .dir = WORLD_DOWN, .mask = COLLAYER_ALL, .radius = explode_radius};
    SolRayResult results[512];
    int hits = Sol_SphereOverlap(w, ray, results, 512);
    for (int i = 0; i < hits; i++)
    {
        int hit_id = results[i].entId;
        if (hit_id == ownerId || Sol_Comp_Has(w, hit_id, ScStage))
            continue;

        vec3s hit_pos = w->xform.pos[hit_id];
        vec3s delta   = vecSub(hit_pos, pos);
        float d2      = glms_vec3_norm2(delta);
        Sol_Ai_QuickLearn(w, a, ownerId, true);
        if (d2 >= 0.000001f)
        {
            float dist = sqrtf(d2);
            vec3s dir  = vecSca(delta, 1.0f / dist);

            SolRayResult los_result = {0};
            bool is_blocked =
                Sol_Raycast1(w,
                             (SolRay){.start     = pos,
                                      .dir       = dir,
                                      .dist      = dist - 0.01f, // Stop slightly short to avoid self-intersection
                                      .mask      = COLLAYER_WORLD,
                                      .ignoreEnt = a},
                             &los_result);
            if (is_blocked)
                continue;
        }

        SolHit aoe_hit = projectile->aoe_hit;
        aoe_hit.entB   = hit_id;
        aoe_hit.pos    = hit_pos;
        Sol_Combat_Hit(w, hit_id, aoe_hit);
    }
    Sol_Event_Push(w, EVENTKIND_HIT,
                   (SolEvent){
                       .entA         = ownerId,
                       .entB         = hit.entB,
                       .as.hit.kind  = HITKIND_FIREBALL_EXPLODE,
                       .as.hit.pos   = pos,
                       .as.hit.power = hit.power,
                   });

    Sol_Destroy_Ent(w, a);
    return true;
}

void Projectile_Step(World *world, double dt)
{
    float fdt                   = (float)dt;
    SparseSet_ScProjectile *set = Sol_Comp_Set(world, ScProjectile);
    for (int i = set->cnt; i-- > 0;)
    {
        int id                   = set->dense[i];
        ScProjectile *projectile = &set->data[i];
        ScBody3 *body3           = Sol_Comp_Get(world, id, ScBody3);
        ScOwner *owner           = Sol_Comp_Get(world, id, ScOwner);
        int ownerId              = owner ? owner->ownerId : 0;
        Xform xform              = Xform_Get(world, id);
        if (xform.pos.y < -15.0f)
            Sol_Destroy_Ent(world, id);

        vec3s vel   = body3->vel;
        float speed = glms_vec3_norm(vel);
        vec3s dir   = (speed > 0.001f) ? vecSca(vel, 1.0f / speed) : (vec3s){0, 0, 1};

        SolRay ray = {
            .start     = vecSub(xform.pos, vecSca(dir, speed * fdt)),
            .dir       = dir,
            .dist      = speed * fdt,
            .mask      = projectile->mask,
            .ignoreEnt = id,
            .radius    = projectile->radius,
        };

        SolRayResult results[16];
        int hits =
            solState.debug ? Sol_SpherecastD(world, ray, results, 16, 0.2f) : Sol_Spherecast(world, ray, results, 16);

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

            Sol_Ai_QuickLearn(world, id, ownerId, false);

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