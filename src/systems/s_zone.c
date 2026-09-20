#include "world.h"
#include "sol_math.h"

static inline void Fireball(World *world, int id, ScZone *zone)
{
    ScOwner owner = *Sol_Comp_Get(world, id, ScOwner);
    int ownerId   = owner.ownerId;
    Xform xform   = Xform_Get(world, id);

    SolRay ray = {.start = xform.pos, .ignoreEnt = id, .dir = WORLD_DOWN, .mask = COLLAYER_ALL, .radius = zone->radius};
    SolRayResult results[256];
    int hits = Sol_SphereOverlap(world, ray, results, 256);
    for (int i = 0; i < hits; i++)
    {
        int hit_id = results[i].entId;
        if (hit_id == ownerId)
            continue;

        Xform hit_xform = Xform_Get(world, hit_id);
        if (!Sol_Hitgen_Try(world, id, hit_id, zone->hitgen))
            continue;

        vec3s normal  = vecNorm(vecSub(hit_xform.pos, xform.pos));
        vec3s hit_pos = Sol_AddScaledDir(ray.start, normal, results[i].t);

        SolHit hit = zone->hit;
        hit.pos    = hit_pos;
        hit.entA   = id;
        hit.entB   = hit_id;
        hit.normal = normal;
        Sol_Combat_Hit(world, hit.entB, hit);

        Sol_Event_Push(world, EVENTKIND_FX,
                       (SolEvent){
                           .as.fx.kind     = EVENTFX_FIREBALL_HIT,
                           .as.fx.pos      = hit.pos,
                           .as.fx.color    = {1.0f, 0.66f, 0.0f, 0.66f},
                           .as.fx.duration = 0.3f,
                       });
    }
}

void Zone_Update(World *world, double dt)
{
    float fdt = (float)dt;

    SparseSet_ScZone *set = Sol_Comp_Set(world, ScZone);

    int count = set->cnt;
    for (int i = 0; i < count; i++)
    {
        int id       = set->dense[i];
        ScZone *zone = &set->data[i];

        zone->accum += fdt;
        while (zone->accum >= zone->rate)
        {
            zone->accum -= zone->rate;

            switch (zone->kind)
            {
            case ZONEKIND_FIREBALL:
                Fireball(world, id, zone);
                break;
            }
        }
    }
}