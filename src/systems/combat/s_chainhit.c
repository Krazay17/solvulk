#include "sol_core.h"

void Chain_Step(World *world, double dt, double time);
int  Find_NextTarget(World *world, Chain *chain);

void Sol_Chainhit_Init(World *world)
{
    world->chainhit           = malloc(sizeof(CompChainhit));
    world->chainhit->capacity = CHAIN_SIZE;
    world->chainhit->count    = 0;
    world->chainhit->chains   = calloc(CHAIN_SIZE, sizeof(Chain));
    WAddStep(world)           = Chain_Step;
}

void Sol_Chainhit_Trigger(World *world, int dealer, int target, u32 count)
{
    Sol_Event_Add(world,
                  (SolEvent){.kind = EVENTKIND_HIT, .as.hit.entA = dealer, .as.hit.entB = target, .as.hit.damage = 10});
    // Sol_Vital_Damage(world, target, dealer, 10);
    Sol_Emitter_Spawn(world, EMITTERKIND_BURST_SPARKS, Sol_Xform_GetPos(world, target), (vec4s){0.7f, 1.0f, 0.0f, 1.0f},
                      0.2f);

    if (count > 0)
    {
        Sol_Realloc(&world->chainhit->chains, world->chainhit->count, &world->chainhit->capacity, sizeof(Chain));
        int idx = world->chainhit->count++;
        memset(&world->chainhit->chains[idx], 0, sizeof(Chain));
        world->chainhit->chains[idx].count           = count;
        world->chainhit->chains[idx].dealer          = dealer;
        world->chainhit->chains[idx].last            = target;
        world->chainhit->chains[idx].hitEnts[target] = true;
        world->chainhit->chains[idx].delay           = 0.1f;
        world->chainhit->chains[idx].accum           = 0.1f;
    }
}

void Chain_Step(World *world, double dt, double time)
{
    int newCount = 0;
    for (int i = 0; i < world->chainhit->count; i++)
    {
        Chain *chain = &world->chainhit->chains[i];
        if (chain->count <= 0)
            continue;
        chain->accum += dt;
        if (chain->accum > chain->delay)
        {
            chain->accum = 0.0f;
            int target   = Find_NextTarget(world, chain);
            if (target > 0)
            {
                Sol_Event_Add(world, (SolEvent){.kind          = EVENTKIND_HIT,
                                                .as.hit.entA   = chain->dealer,
                                                .as.hit.entB   = target,
                                                .as.hit.damage = 10});

                // Draw a visual ribbon from old target positions to the new target
                Sol_Ribbon_AddBetweenEntities(world, chain->last, target, RIBBONKIND_TRAIL, 1.0f,
                                              (vec4s){0.8f, 1.0f, 0.5f, 1.0f});
                Sol_Emitter_Spawn(world, EMITTERKIND_BURST_SPARKS, Sol_Xform_GetPos(world, target),
                                  (vec4s){0.8f, 1.0f, 0.5f, 1.0f}, 0.2f);

                // Update tracker state data
                chain->hitEnts[target] = true;
                chain->last            = target;
                chain->count--;
            }
            else
                chain->count = 0;
        }
        if (chain->count > 0)
        {
            world->chainhit->chains[newCount++] = *chain;
        }
    }
    world->chainhit->count = newCount;
}

int Find_NextTarget(World *world, Chain *chain)
{
    SolRayResult results[64] = {0};
    SolRay       ray         = {.pos = Sol_Xform_GetPos(world, chain->last), .mask = 1, .ignoreEnt = chain->dealer};
    int          hits        = Sol_SphereCast(world, ray, 10.0f, results, 64);
    float        closest     = 999999.9f;
    int          closestId   = 0;
    for (int i = 0; i < hits; i++)
    {
        SolRayResult result = results[i];
        if (!Sol_Owner_GetHostile(world, chain->dealer, result.entId))
            continue;
        if (result.entId == chain->last)
            continue;
        if (chain->hitEnts[result.entId])
            continue;
        float distance = Sol_Xform_DistanceTo2(world, chain->last, result.entId);
        if (distance < closest)
        {
            closest   = distance;
            closestId = result.entId;
        }
    }

    if (closestId > 0)
    {
        return closestId;
    }
    return 0;
}