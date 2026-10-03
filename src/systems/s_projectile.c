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
        if (!Sol_Combat_Hostile(w, a, hit_id))
            continue;

        vec3s hit_pos = w->xform.pos[hit_id];
        vec3s delta   = vecSub(hit_pos, pos);
        float d2      = glms_vec3_norm2(delta);
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
        Sol_Ai_QuickLearn(w, a, ownerId, false);
    }
    Sol_Event_Push(w, EVENTKIND_HIT,
                   (SolEvent){
                       .entA         = ownerId,
                       .entB         = hit.entB,
                       .as.hit.kind  = HITKIND_FIREBALL_EXPLODE,
                       .as.hit.pos   = pos,
                       .as.hit.power = hit.power,
                   });

    return true;
}

static inline isDestroyed PlasmaOrbHit(World *world, int a, ScProjectile *projectile, SolHit hit)
{
    ScOwner *owner = Sol_Comp_Get(world, a, ScOwner);
    int ownerId    = owner ? owner->ownerId : 0;

    if (Sol_Hitgen_Try(world, a, hit.entB, projectile->hitgen))
    {

        Sol_Combat_Hit(world, hit.entB, hit);

        Sol_Event_Push(world, EVENTKIND_HIT,
                       (SolEvent){
                           .entA         = ownerId,
                           .entB         = hit.entB,
                           .as.hit.kind  = HITKIND_FIREBALL_EXPLODE,
                           .as.hit.pos   = hit.pos,
                           .as.hit.power = hit.power,
                       });
    }
    if (Sol_Comp_Has(world, hit.entB, ScStage))
        return true;

    return false;
}

static inline isDestroyed LightningBoltHit(World *world, int id, ScProjectile *p, SolHit hit)
{
    if (Sol_Combat_Hostile(world, id, hit.entB) && Sol_Hitgen_Try(world, id, hit.entB, p->hitgen))
    {
        Sol_Combat_Hit(world, hit.entB, hit);

        ScBody3 *body3 = Sol_Comp_Get(world, id, ScBody3);
        body3->vel     = GLMS_VEC3_ZERO;

        // 1. Snap projectile position directly to hit impact point
        world->xform.pos[id] = hit.pos;

        // 2. Fetch target transform
        vec3s target_pos     = world->xform.pos[hit.entB];
        versors target_rot   = world->xform.rot[hit.entB];
        versors inv_target_r = glms_quat_inv(target_rot);

        // 3. Compute local space offset & rotation (Child_Local = Parent_Inv * Child_World)
        vec3s world_delta  = glms_vec3_sub(hit.pos, target_pos);
        vec3s local_offset = glms_quat_rotatev(inv_target_r, world_delta);
        versors local_quat = glms_quat_mul(inv_target_r, world->xform.rot[id]);

        // 4. Attach ScParent
        *Sol_Comp_Add(world, id, ScParent) = (ScParent){
            .parentId    = hit.entB,
            .localOffset = local_offset,
            .localQuat   = local_quat,
            .active      = true,
        };

        Sol_Event_Push(world, EVENTKIND_HIT,
                       (SolEvent){
                           .entB         = hit.entB,
                           .as.hit.kind  = HITKIND_NORMAL,
                           .as.hit.pos   = hit.pos,
                           .as.hit.power = hit.power,
                       });

        Sol_Comp_Rem(world, id, ScProjectile);
    }

    return false;
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
        if (!body3)
            continue;
        Xform xform = Xform_Get(world, id);
        if (xform.pos.y < -15.0f || fabs(xform.pos.x) > 500.0f || fabs(xform.pos.z) > 500.0f)
        {
            Sol_Destroy_Ent(world, id);
            continue;
        }

        vec3s vel    = body3->vel;
        float v2     = glms_vec3_norm2(body3->vel);
        if (v2 > 0.1f)
        {
            vec3s dir            = glms_vec3_scale(body3->vel, 1.0f / sqrt(v2));
            vec3s base_dir       = {0.0f, 1.0f, 0.0f};
            world->xform.rot[id] = glms_quat_from_vecs(base_dir, dir);
        }

        float speed = glms_vec3_norm(vel);
        vec3s dir   = (speed > 0.001f) ? vecSca(vel, 1.0f / speed) : (vec3s){0, 0, 1};

        SolRay ray = {
            .start     = vecSub(xform.pos, vecSca(vel, fdt)),
            .dir       = dir,
            .dist      = speed * fdt,
            .mask      = (COLLAYER_ALL & ~COLLAYER_PROJECTILE),
            .ignoreEnt = id,
            .radius    = projectile->radius,
        };

        SolRayResult results[16];
        int hits = solState.debug ? Sol_SpherecastD(world, ray, results, 16, 0.2f)
                                  : Sol_Spherecast(world, ray, results, 16);
        if (hits == 0)
            continue;

        ScOwner *owner = Sol_Comp_Get(world, id, ScOwner);
        int ownerId    = owner ? owner->ownerId : 0;

        SolHit hit = projectile->hit;
        hit.entA   = id;
        hit.vel    = glms_vec3_normalize(vel);
        for (int j = 0; j < hits; j++)
        {
            SolRayResult result = results[j];

            // Skip owner collision
            if (result.entId == ownerId)
                continue;

            hit.entB   = result.entId;
            hit.normal = result.norm;
            hit.pos    = result.pos;

            Sol_Ai_QuickLearn(world, id, ownerId, false);

            bool destroyed = false;
            switch (projectile->kind)
            {
            case PROJECTILEKIND_FIREBALL:
                destroyed = FireballHit(world, id, projectile, hit);
                break;
            case PROJECTILEKIND_PLASMAORB:
                destroyed = PlasmaOrbHit(world, id, projectile, hit);
                break;
            case PROJECTILEKIND_LIGHTNINGBOLT:
                destroyed = LightningBoltHit(world, id, projectile, hit);
                break;
            }
            if (projectile->hook)
                projectile->hook(world, id, result.entId);

            if (destroyed)
            {
                Sol_Destroy_Ent(world, id);
                break;
            }
        }
    }
}

// void Sol_Projectile_Reflect(World *world, int attacker, float dt, vec3s a0, vec3s a1, float radius)
// {
//     SparseSet_ScProjectile *set = Sol_Comp_Set(world, ScProjectile);
//     for (int i = set->cnt; i-- > 0;)
//     {
//         int id = set->dense[i];
//         if (!Sol_Combat_Hostile(world, attacker, id))
//             continue;
//         ScProjectile *projectile = &set->data[i];
//         ScBody3 *body3           = Sol_Comp_Get(world, id, ScBody3);
//         vec3s vel                = body3->vel;

//         vec3s b0 = world->xform.pos[id];
//         vec3s b1 = glms_vec3_add(b0, glms_vec3_scale(vel, dt));

//         CapHit hit;
//         if (Sol_CapsuleOverlap(world, a0, a1, b0, b1, radius, projectile->radius, &hit))
//         {
//             ScTeam *team          = Sol_Comp_Get(world, id, ScTeam);
//             ScOwner *owner        = Sol_Comp_Get(world, id, ScOwner);
//             ScTeam *attacker_team = Sol_Comp_Get(world, attacker, ScTeam);
//             ScCmd *cmd            = Sol_Comp_Get(world, attacker, ScCmd);

//             if (owner)
//                 owner->ownerId = attacker;
//             if (team)
//                 team->team = attacker_team->team;

//             if (body3 && cmd)
//             {
//                 body3->vel       = Sol_RedirectVel(body3->vel, cmd->aimdir);
//                 body3->ignoreEnt = attacker;
//             }

//             Sol_Event_Push(world, EVENTKIND_FX, (SolEvent){.as.fx.kind = FXKIND_PARRY, .as.fx.pos = hit.contactA});
//         }
//     }
// }