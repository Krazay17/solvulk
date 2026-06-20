#include "s_combat.h"
#include "sol_core.h"
#include "world.h"
#include "sol_math.h"
#include "audio.h"
#include "xform/s_xform.h"
#include "controller/s_controller.h"
#include "combat/s_combat.h"
#include "model/s_model.h"
#include "event/s_event.h"
#include "physx/s_body.h"

#define CHAIN_CAP 0xff

typedef struct
{
    vec4s color;
    u32   chainCount;
    u8    fxKind;
} ChainConfig;

static const ChainConfig chain_config[] = {
    [CHAINKIND_LIGHTNING] = {.color = {.5f, .5f, 1.0f, 1.0f}, .chainCount = 10, .fxKind = FXKIND_CHAINLIGHTNING},
};

void Chain_Step(World *world, double dt, double time);
int  Find_NextTarget(World *world, Chain *chain);

void Sol_Chainhit_Init(World *world)
{
    world->chainhit           = malloc(sizeof(ChainAttacks));
    world->chainhit->capacity = CHAIN_CAP;
    world->chainhit->count    = 0;
    world->chainhit->chains   = calloc(CHAIN_CAP, sizeof(Chain));
    WAddStep(world)           = Chain_Step;
}

void Sol_Chainhit_Trigger(World *world, int dealer, int target, u32 kind, float damage)
{
    Sol_Event_Add(world, (SolEvent){
                             .kind          = EVENTKIND_HIT,
                             .as.hit.entA   = dealer,
                             .as.hit.entB   = target,
                             .as.hit.pos    = Sol_Xform_GetPos(world, target),
                             .as.hit.damage = damage,
                             .as.hit.fxKind = FXKIND_LIGHTNING,
                         });


    Sol_Realloc(&world->chainhit->chains, world->chainhit->count, &world->chainhit->capacity, sizeof(Chain));
    int idx = world->chainhit->count++;
    memset(&world->chainhit->chains[idx], 0, sizeof(Chain));
    world->chainhit->chains[idx].count           = chain_config[kind].chainCount;
    world->chainhit->chains[idx].damage          = damage;
    world->chainhit->chains[idx].dealer          = dealer;
    world->chainhit->chains[idx].last            = target;
    world->chainhit->chains[idx].hitEnts[target] = true;
    world->chainhit->chains[idx].delay           = 0.0666f;
    world->chainhit->chains[idx].accum           = 0.0666f;
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
                Sol_Event_Add(world, (SolEvent){
                                         .kind          = EVENTKIND_HIT,
                                         .as.hit.entA   = chain->dealer,
                                         .as.hit.entB   = target,
                                         .as.hit.damage = chain->damage,
                                     });
                Sol_Event_Add(world, (SolEvent){
                                         .kind       = EVENTKIND_FX,
                                         .as.fx.kind = chain_config[chain->kind].fxKind,
                                         .as.fx.entA = chain->last,
                                         .as.fx.entB = target,
                                         .as.fx.pos  = Sol_Xform_GetPos(world, target),
                                     });
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
    SolRayResult results[256];
    SolRay       ray       = {.pos = Sol_Xform_GetPos(world, chain->last), .mask = 1, .ignoreEnt = chain->dealer};
    int          hits      = Sol_SphereCast(world, ray, 10.0f, results, 256);
    float        closest   = 999999.9f;
    int          closestId = 0;
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