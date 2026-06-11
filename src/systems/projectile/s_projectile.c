#include "sol_core.h"

static void Projectile_Step(World *world, double dt, double time);
static void Projectile_Hit(World *world, int id, SolHit hit);
static bool Projectile_Bounce(World *world, int id);

void Sol_Projectile_Init(World *world)
{
    world->projectiles = calloc(MAX_ENTS, sizeof(CompProjectile));
    WAddStep(world)    = Projectile_Step;
}

CompProjectile *Sol_Projectile_Add(World *world, int id, ProjectileKind kind, float power)
{
    CompProjectile projectile = {0};
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
        CompProjectile *projectile = &world->projectiles[proj];
        SolHit          hit        = projectile->directHit;
        hit.entB                   = other;
        hit.pos                    = e->as.collision.pos;
        hit.vel                    = e->as.collision.vel;
        hit.normal                 = e->as.collision.normal;

        Projectile_Hit(world, proj, hit);
        Projectile_Bounce(world, proj);
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
            CompProjectile *projectile = &world->projectiles[id];
            SolHit          hit        = projectile->directHit;
            hit.entB                   = result.entId;
            hit.pos                    = result.pos;
            hit.normal                 = result.norm;
            hit.vel                    = body->vel;

            Projectile_Hit(world, id, hit);

            if (!Projectile_Bounce(world, id))
                break;
        }
    }
}

static bool Projectile_Bounce(World *world, int id)
{
    CompProjectile *projectile = &world->projectiles[id];
    if (projectile->bounces < 1)
    {
        Sol_Destroy_Ent(world, id);
        return false;
    }
    else
        projectile->bounces--;
    return true;
}

static void Projectile_Hit(World *world, int id, SolHit hit)
{
    CompProjectile *projectile = &world->projectiles[id];
    SolHit          aoeHit     = projectile->explosionHit;
    hit.entA = aoeHit.entA = Sol_Owner_GetOwner(world, id);
    hit.power = aoeHit.power = projectile->power;

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

        SolRay       ray = {.pos = pos, .ignoreEnt = id};
        SolRayResult results[256];
        int          hits = Sol_SphereCast(world, ray, radius, results, 256);
        for (int i = 0; i < hits; i++)
        {
            SolRayResult result = results[i];
            vec3s        dir    = glms_vec3_normalize(glms_vec3_sub(result.pos, pos));
            float        dist   = glms_vec3_distance(result.pos, pos);

            SolRayResult losCheck =
                Sol_RaycastD(world, (SolRay){.pos = pos, .dir = dir, .dist = dist, .ignoreEnt = id, .mask = 0}, 1.0f);
            if (losCheck.hit)
                continue;
            aoeHit.entB = result.entId;
            aoeHit.pos  = result.pos;
            aoeHit.vel  = vecSub(result.pos, pos);
            Sol_Event_Add(world, (SolEvent){.kind = EVENTKIND_HIT, .as.hit = aoeHit});
        }
    }
}