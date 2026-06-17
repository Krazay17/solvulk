#include "sol_core.h"

void Chain_Lightning_Recursive(World *world, int dealer, int target, int last, float damage, int count)
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
        //Sol_Ribbon_AddBetweenEntities(world, target, closestId, RIBBONKIND_TRAIL, 1.0f,
        //                              (vec4s){0.7f, 1.0f, 0.0f, 1.0f});
        Sol_Emitter_Spawn(world, EMITTERKIND_BURST_SPARKS, Sol_Xform_GetPos(world, closestId),
                          (vec4s){0.7f, 1.0f, 0.0f, 1.0f}, 0.2f);
        Chain_Lightning(world, dealer, closestId, target, damage, count);
    }
}