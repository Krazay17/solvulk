#include "s_zone.h"
#include "world.h"
#include "xform/s_xform.h"
#include "owner/s_owner.h"
#include "physx/s_body.h"
#include "render/render.h"
#include "sol_core.h"
#include "combat/s_combat.h"

#define MAX_HITS 64

CompZone zone_kinds[ZONEKIND_COUNT] = {
    [ZONEKIND_FIRE] =
        {
            .kind     = ZONEKIND_FIRE,
            .duration = 5.0f,
            .rate     = 0.2f,
            .value    = 5.0f,
            .radius   = 2.0f,
        },
    [ZONEKIND_HEAL] =
        {
            .kind     = ZONEKIND_HEAL,
            .duration = 5.0f,
            .rate     = 0.05f,
            .value    = 1.0f,
            .radius   = 2.0f,
        },
};

void Zone_Draw(World *world, double dt, double time)
{
}

static const u64 required = BITC(HAS_ZONE);

void Zone_Step(World *world, double dt, double time)
{
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if (!(world->masks[id] & required))
            continue;
        CompZone  *zone  = &world->zones[id];
        CompXform *xform = &world->xforms[id];

        zone->accum += dt;
        if (zone->accum >= zone->rate)
        {
            zone->accum      = 0;
            CompOwner *owner = &world->owners[id];

            SolRayResult results[MAX_HITS];
            int          hits = Sol_SphereCast(world,
                                               (SolRay){
                                                   .pos = xform->pos,
                                               },
                                               zone->radius, results, MAX_HITS);
            for (int j = 0; j < hits; j++)
            {
                SolRayResult result = results[j];
                switch (zone->kind)
                {
                case ZONEKIND_FIRE:
                    Sol_Combat_ApplyHit(world, result.entId,
                                        (SolHit){
                                            .entA   = id,
                                            .pos    = result.pos,
                                            .damage = zone->value,
                                        });
                    break;
                case ZONEKIND_HEAL:
                    Sol_Combat_ApplyHeal(world, result.entId,
                                         (SolHit){
                                             .entA   = id,
                                             .pos    = result.pos,
                                             .damage = zone->value,
                                             .buffMask = BITC(BUFFKIND_HOT),
                                         });
                    break;
                }
            }
        }
    }
}

void Sol_Zone_Init(World *world)
{
    world->zones    = calloc(MAX_ENTS, sizeof(CompZone));
    WAddStep(world) = Zone_Step;
    WAdd3d(world)   = Zone_Draw;
}

void Sol_Zone_Add(World *world, int id, ZoneKind kind)
{
    CompZone *zone = &world->zones[id];
    *zone          = zone_kinds[kind];

    world->masks[id] |= BITC(HAS_ZONE);
}