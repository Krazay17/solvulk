#include "sol_core.h"

void Chain_Lightning(World *world, int dealer, int target, int last, u32 damage, int count)
{
    Sol_Vital_Damage(world, target, dealer, damage);
    count--;
    if (count < 1)
        return;
    if (count >= 64)
    {
        Chain_Lightning(world, dealer, target, 0, damage, count);
        return;
    }

    SolRayResult results[64] = {0};
    SolRay       ray         = {.pos = Sol_Xform_GetPos(world, target), .mask = 1, .ignoreEnt = target};
    int          hits        = Sol_SphereCast(world, ray, 5.0f, results, count);
    float        closest     = 999999.9f;
    float        farthest    = 0;
    int          closestId   = 0;
    for (int i = 0; i < hits; i++)
    {
        SolRayResult result = results[i];
        if (!Sol_Owner_GetHostile(world, dealer, result.entId))
            continue;
        if (result.entId == last)
            continue;
        float distance = Sol_Xform_DistanceTo2(world, target, result.entId);
        if (distance > farthest)
        {
            closest   = distance;
            farthest  = distance;
            closestId = result.entId;
        }
    }

    if (closestId > 0)
    {
        Sol_Ribbon_AddBetweenEntities(world, target, closestId, RIBBONKIND_TRAIL, 1.0f,
                                      (vec4s){0.7f, 1.0f, 0.0f, 1.0f});
        Sol_Emitter_Spawn(world, EMITTERKIND_BURST_SPARKS, Sol_Xform_GetPos(world, closestId),
                          (vec4s){0.7f, 1.0f, 0.0f, 1.0f}, 0.2f);
        Chain_Lightning(world, dealer, closestId, target, damage, count);
    }
}

void Chain_Lightning_NoReassess(World *world, int dealer, int target, u32 damage, int count)
{
    int          i, j;
    SolRayResult results[64]   = {0};
    SolRay       ray           = {.pos = Sol_Xform_GetPos(world, target), .mask = 1, .ignoreEnt = target};
    int          hits          = Sol_SphereCast(world, ray, 5.0f, results, count);
    float        closest       = 999999.9f;
    float        farthest      = 0;
    int          closestId     = 0;
    int          sortedIds[64] = {0};
    int          sortCount     = 0;

#pragma omp parallel for if (count > 20) schedule(dynamic, 16)
    for (i = 0; i < hits; i++)
    {
        SolRayResult result = results[i];
        if (!Sol_Owner_GetHostile(world, dealer, result.entId))
            continue;
        sortedIds[i] = result.entId;
        sortCount++;
    }
    for (i = 0; i < sortCount; i++)
    {
        Sol_Ribbon_AddBetweenEntities(world, target, sortedIds[i], RIBBONKIND_TRAIL, 1.0f,
                                      (vec4s){0.7f, 1.0f, 0.0f, 1.0f});
        Sol_Emitter_Spawn(world, EMITTERKIND_BURST_SPARKS, Sol_Xform_GetPos(world, sortedIds[i]),
                          (vec4s){0.7f, 1.0f, 0.0f, 1.0f}, 0.2f);
    }
}

void Chain_Lightning_Single(World *world, int dealer, int last, u32 count)
{
    SolRayResult results[64] = {0};
    SolRay       ray         = {.pos = Sol_Xform_GetPos(world, last), .mask = 1, .ignoreEnt = last};
    int          hits        = Sol_SphereCast(world, ray, 5.0f, results, count);
    float        closest     = 999999.9f;
    float        farthest    = 0;
    int          closestId   = 0;
    for (int i = 0; i < hits; i++)
    {
        SolRayResult result = results[i];
        if (!Sol_Owner_GetHostile(world, dealer, result.entId))
            continue;
        float distance = Sol_Xform_DistanceTo2(world, last, result.entId);
        if (distance > farthest)
        {
            closest   = distance;
            farthest  = distance;
            closestId = result.entId;
        }
    }

    if (closestId > 0)
    {
        count--;
        CompCombat *combat = &world->combats[dealer];
        // world->combats->chainLightning.hitEnts[closestId] = true;
        combat->chainLightning.last  = closestId;
        combat->chainLightning.delay = 0.15f;
        combat->chainLightning.count = count;
        combat->chainLightning.accum = 0;

        Sol_Vital_Damage(world, closestId, dealer, 10);
        Sol_Ribbon_AddBetweenEntities(world, last, closestId, RIBBONKIND_TRAIL, 1.0f, (vec4s){0.7f, 1.0f, 0.0f, 1.0f});
        Sol_Emitter_Spawn(world, EMITTERKIND_BURST_SPARKS, Sol_Xform_GetPos(world, closestId),
                          (vec4s){0.7f, 1.0f, 0.0f, 1.0f}, 0.2f);
    }
}