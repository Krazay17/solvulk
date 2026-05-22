#include "sol_core.h"

typedef struct CompContact
{
    ImpactList impacts;
    u32        bounces;
} CompContact;

static void Contact_Step(World *world, double dt, double time);

void Sol_Contact_Init(World *world)
{
    WAddStep(world) = Contact_Step;
    world->contacts = calloc(MAX_ENTS, sizeof(CompContact));
}

void Sol_Contact_Add(World *world, int id, ContactDesc desc)
{
    world->masks[id] |= HAS_CONTACT;
    CompContact *c = &world->contacts[id];
    c->impacts     = desc.impacts;
    c->bounces     = desc.bounces;
}

static void Contact_Step(World *world, double dt, double time)
{
    for (int i = 0; i < world->events->count; i++)
    {
        SolEvent *e   = &world->events->event[i];
        vec3s     pos = e->as.collision.pos;
        switch (e->kind)
        {
        case EVENTKIND_COLLISION:
            int proj, other;
            if (world->masks[e->as.collision.entA] & HAS_CONTACT)
            {
                proj  = e->as.collision.entA;
                other = e->as.collision.entB;
            }
            else if (world->masks[e->as.collision.entB] & HAS_CONTACT)
            {
                proj  = e->as.collision.entB;
                other = e->as.collision.entA;
            }
            else
                continue;

            if (world->owners[proj].ownerId == other)
                continue;
            CompContact *c = &world->contacts[proj];
            vec3s projPos = Sol_Xform_GetPos(world, proj);

            for (int i = 0; i < c->impacts.impactCount; i++)
            {
                Impact *impact     = &c->impacts.impacts[i];
                impact->hit.source = Sol_Owner_GetOwner(world, proj);

                if (impact->kind == IMPACT_DIRECT)
                {
                    impact->hit.vel    = Sol_Physx_GetVel(world, proj);
                    impact->hit.target = other;
                    if (Sol_Combat_IsReflecting(world, other))
                    {
                        Sol_Owner_SetOwner(world, proj, other);
                        Sol_Physx_SetVel(
                            world, proj,
                            Sol_RedirectVel(Sol_Physx_GetVel(world, proj), Sol_Controller_GetAimdir(world, other)));
                        goto breakout;
                    }
                    impact->hit.pos = pos;
                    impact->hit.dir = vecSub(Sol_Xform_GetPos(world, other), Sol_Xform_GetPos(world, proj));
                    Sol_Event_Add(world, (SolEvent){.kind = EVENTKIND_HIT, .as.hit = impact->hit});
                    Sol_Event_Add(
                        world, (SolEvent){.kind = EVENTKIND_FX, .as.fx.pos = projPos, .as.fx.kind = FXKIND_FIREBALL_HIT, .as.fx.scale = impact->radius});
                }
                else if (impact->kind == IMPACT_AOE)
                {
                    SolRayResult results[256];

                    int hits = Sol_SphereCast(world, (SolRay){.pos = projPos}, impact->radius, results, 256);
                    for (int i = 0; i < hits; i++)
                    {
                        SolRayResult result = results[i];
                        if ((world->masks[result.entId] & HAS_VITAL) && world->owners[proj].ownerId != result.entId)
                        {
                            impact->hit.target = result.entId;
                            impact->hit.pos    = result.pos;
                            impact->hit.dir    = vecSub(Sol_Xform_GetPos(world, other), Sol_Xform_GetPos(world, proj));
                            Sol_Event_Add(world, (SolEvent){.kind = EVENTKIND_HIT, .as.hit = impact->hit});
                        }
                    }
                }
            }

            if (c->bounces < 1)
                Sol_Destroy_Ent(world, proj);
            else
                c->bounces--;
            break;

        breakout:;
        }
    }
}
