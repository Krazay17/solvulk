#include "sol_core.h"

static void Projectile_Step(World *world, double dt, double time);
static void Projectile_Hit(World *world, int id, SolHit hit);

static CompProjectile projectile_kinds[PROJECTILEKIND_COUNT] = {
    [PROJECTILEKIND_BULLET] =
        {
            .directHitKind = HITKIND_BULLET,
            .bounces       = 1,
        },
    [PROJECTILEKIND_FIREBALL] =
        {
            .directHitKind    = HITKIND_FIREBALL,
            .explodeRadius    = 2.5f,
            .explosionHitKind = HITKIND_FIREBALL_EXPLODE,
        },
};

void Sol_Projectile_Init(World *world)
{
    world->projectiles = calloc(MAX_ENTS, sizeof(CompProjectile));
    WAddStep(world)    = Projectile_Step;
}

CompProjectile *Sol_Projectile_Add(World *world, int id, ProjectileKind kind, float power)
{
    CompProjectile projectile = projectile_kinds[kind];
    projectile.power          = power;

    world->projectiles[id] = projectile;
    world->masks[id] |= HAS_PROJECTILE;
    return &world->projectiles[id];
}

static void Projectile_Step(World *world, double dt, double time)
{
    int required = HAS_PROJECTILE;
    for (int i = 0; i < world->events->count; i++)
    {
        SolEvent *e = &world->events->event[i];
        if (e->kind != EVENTKIND_COLLISION)
            continue;
        int proj, other;
        if (world->masks[e->as.collision.entA] & HAS_PROJECTILE)
        {
            proj  = e->as.collision.entA;
            other = e->as.collision.entB;
        }
        else if (world->masks[e->as.collision.entB] & HAS_PROJECTILE)
        {
            proj  = e->as.collision.entB;
            other = e->as.collision.entA;
        }
        else
            continue;
        if (Sol_Owner_GetOwner(world, proj) == other)
            continue;

        SolHit hit = {
            .entB   = other,
            .pos    = e->as.collision.pos,
            .vel    = e->as.collision.vel,
            .normal = e->as.collision.normal,
        };
        Projectile_Hit(world, proj, hit);
    }
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;
        CompXform *xform = &world->xforms[id];
        CompBody  *body  = &world->bodies[id];
        float      dist  = glms_vec3_distance(xform->lastPos, xform->pos);
        vec3s      dir   = glms_vec3_normalize(body->vel);
        SolRay     ray   = {
            .dir       = dir,
            .dist      = dist,
            .pos       = xform->lastPos,
            .ignoreEnt = id,
        };
        SolRayResult results[24] = {0};
        int          hits        = Sol_SphereCast(world, ray, body->dims.x, results, 24);
        for (int i = 0; i < hits; i++)
        {
            SolRayResult result = results[i];
            if (!Sol_Owner_GetHostile(world, id, result.entId))
                continue;

            SolHit hit = {
                .entB   = result.entId,
                .pos    = result.pos,
                .normal = result.norm,
                .vel    = body->vel,
            };
            Projectile_Hit(world, id, hit);
        }
    }
}

static void Projectile_Hit(World *world, int id, SolHit hit)
{
    CompProjectile *projectile = &world->projectiles[id];
    hit.entA                   = Sol_Owner_GetOwner(world, id);
    hit.kind                   = projectile->directHitKind;
    hit.power                  = projectile->power;

    Sol_Event_Add(world, (SolEvent){
                             .kind   = EVENTKIND_HIT,
                             .as.hit = hit,
                         });

    if (projectile->explodeRadius > 0)
    {
        CompXform *xform = &world->xforms[id];
        float      radius =
            projectile->power > 0 ? projectile->explodeRadius * projectile->power : projectile->explodeRadius;
        vec3s pos = xform->pos;
        hit.kind  = projectile->explosionHitKind;

        SolRay       ray = {.pos = pos};
        SolRayResult results[256];
        int          hits = Sol_SphereCast(world, ray, radius, results, 256);
        for (int i = 0; i < hits; i++)
        {
            SolRayResult result = results[i];
            hit.entB            = result.entId;
            hit.pos             = result.pos;
            hit.vel             = vecSub(result.pos, pos);
            Sol_Event_Add(world, (SolEvent){.kind = EVENTKIND_HIT, .as.hit = hit});
        }
    }

    if (projectile->bounces < 1)
        Sol_Destroy_Ent(world, id);
    else
        projectile->bounces--;
}