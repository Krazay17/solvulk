#include "sol_core.h"

const ImpactList contact_config[CONTACTKIND_COUNT] = {
    [CONTACTKIND_FIREBALL] =
        {
            .impactCount = 2,
            .impacts[0] =
                {
                    .kind   = IMPACT_DIRECT,
                    .radius = 1.0f,
                    .hit =
                        {
                            .kind   = HITKIND_FIRE,
                            .damage = 30,
                        },
                },
            .impacts[1] =
                {
                    .kind   = IMPACT_AOE,
                    .radius = 3.0f,
                    .hit =
                        {
                            .damage    = 20,
                            .kind      = HITKIND_FIRE,
                            .buffKind   = BUFFKIND_FIRE,
                            .power     = 10.0f,
                            .buffcount = 1,
                        },
                },
        },
    [CONTACTKIND_WIZARDTOUCH] =
        {
            .impactCount = 1,
            .impacts[0] =
                {
                    .kind   = IMPACT_DIRECT,
                    .radius = 0.5f,
                    .hit =
                        {
                            .kind   = HITKIND_FIRE,
                            .damage = 15,
                        },
                },
        },
};

static void Contact_Step(World *world, double dt, double time);

void Sol_Contact_Init(World *world)
{
    WAddStep(world) = Contact_Step;
    world->contacts = calloc(MAX_ENTS, sizeof(CompContact));
}

void Sol_Contact_Add(World *world, int id, ContactKind kind, float damageScale)
{
    CompContact contact = {
        .kind        = kind,
        .damageScale = damageScale,
        .radiusScale = 1.0f,
        .bounces     = 0,
    };

    world->masks[id] |= HAS_CONTACT;
    world->contacts[id] = contact;
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

            CompContact *c           = &world->contacts[proj];
            vec3s        projPos     = Sol_Xform_GetPos(world, proj);
            u32          impactCount = contact_config[c->kind].impactCount;

            for (int i = 0; i < impactCount; i++)
            {
                const Impact *impact     = &contact_config[c->kind].impacts[i];
                SolHit        dynamicHit = impact->hit;
                float         radius     = impact->radius;
                radius *= c->radiusScale;

                dynamicHit.damage = (u32)((float)impact->hit.damage * c->damageScale);

                dynamicHit.source = Sol_Owner_GetOwner(world, proj);

                if (impact->kind == IMPACT_DIRECT)
                {
                    dynamicHit.vel    = Sol_Physx_GetVel(world, proj);
                    dynamicHit.target = other;
                    if (Sol_Combat_IsReflecting(world, other))
                    {
                        Sol_Owner_SetOwner(world, proj, other);
                        Sol_Physx_SetVel(
                            world, proj,
                            Sol_RedirectVel(Sol_Physx_GetVel(world, proj), Sol_Controller_GetAimdir(world, other)));
                        goto breakout;
                    }
                    dynamicHit.pos = pos;
                    dynamicHit.dir = vecSub(Sol_Xform_GetPos(world, other), Sol_Xform_GetPos(world, proj));
                    Sol_Event_Add(world, (SolEvent){.kind = EVENTKIND_HIT, .as.hit = dynamicHit});
                    Sol_Event_Add(world, (SolEvent){.kind        = EVENTKIND_FX,
                                                    .as.fx.pos   = projPos,
                                                    .as.fx.kind  = FXKIND_FIREBALL_HIT,
                                                    .as.fx.scale = radius});
                }
                else if (impact->kind == IMPACT_AOE)
                {
                    SolRayResult results[256];

                    int hits = Sol_SphereCast(world, (SolRay){.pos = projPos}, radius, results, 256);
                    for (int i = 0; i < hits; i++)
                    {
                        SolRayResult result = results[i];
                        if ((world->masks[result.entId] & HAS_VITAL) && world->owners[proj].ownerId != result.entId)
                        {
                            dynamicHit.target = result.entId;
                            dynamicHit.pos    = result.pos;
                            dynamicHit.dir    = vecSub(Sol_Xform_GetPos(world, other), Sol_Xform_GetPos(world, proj));
                            Sol_Event_Add(world, (SolEvent){.kind = EVENTKIND_HIT, .as.hit = dynamicHit});
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

void Sol_Contact_SetBounces(World *world, int id, int bounces)
{
    world->contacts[id].bounces = bounces;
}

void Sol_Contact_SetRadiusScale(World *world, int id, float radiusScale)
{
    world->contacts[id].radiusScale = radiusScale;
}