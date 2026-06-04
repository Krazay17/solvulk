#include "sol_core.h"

#include "body2d_i.h"

Resolver resolver_kinds[BODY2DKIND_COUNT] = {
    [BODY2DKIND_RECT] = ResolveRect,
};

void ResolveRect(World *world, vec2s *posA, CompBody2d *bodyA, vec2s *posB, CompBody2d *bodyB)
{
    vec2s *velA = &bodyA->vel;
    vec2s *velB = &bodyB->vel;

    // 1. Check for overlap on X and Y axes
    bool overlapX = (posA->x < posB->x + bodyB->dims.x) && (posA->x + bodyA->dims.x > posB->x);
    bool overlapY = (posA->y < posB->y + bodyB->dims.y) && (posA->y + bodyA->dims.y > posB->y);

    if (!overlapX || !overlapY)
        return;

    // 2. Calculate penetration depths
    float overlapLeft  = (posA->x + bodyA->dims.x) - posB->x;
    float overlapRight = (posB->x + bodyB->dims.x) - posA->x;
    float overlapTop   = (posA->y + bodyA->dims.y) - posB->y;
    float overlapBot   = (posB->y + bodyB->dims.y) - posA->y;

    float minX = (overlapLeft < overlapRight) ? overlapLeft : overlapRight;
    float minY = (overlapTop < overlapBot) ? overlapTop : overlapBot;

    // 3. Determine Collision Normal and Push Out equally (0.5 to each)
    vec2s normal = {0, 0};
    if (minX < minY)
    {
        if (overlapLeft < overlapRight)
        {
            posA->x -= overlapLeft * 0.5f;
            posB->x += overlapLeft * 0.5f;
            normal.x = -1.0f;
        }
        else
        {
            posA->x += overlapRight * 0.5f;
            posB->x -= overlapRight * 0.5f;
            normal.x = 1.0f;
        }
    }
    else
    {
        if (overlapTop < overlapBot)
        {
            posA->y -= overlapTop * 0.5f;
            posB->y += overlapTop * 0.5f;
            normal.y = -1.0f;
        }
        else
        {
            posA->y += overlapBot * 0.5f;
            posB->y -= overlapBot * 0.5f;
            normal.y = 1.0f;
        }
    }

    // 4. Elastic Impulse Velocity Transfer
    // Calculate relative velocity along our contact normal
    vec2s relVel         = glms_vec2_sub(*velA, *velB);
    float velAlongNormal = glms_vec2_dot(relVel, normal);

    // Only resolve if they are moving towards each other
    if (velAlongNormal < 0)
    {
        float bounciness = 0.5f; // Restitution coefficient

        // Total impulse scalar (assuming equal mass of 1.0 for UI objects)
        // Formula: j = -(1 + e) * (vA - vB) . n / (1/mA + 1/mB)
        float impulseScalar = -(1.0f + bounciness) * velAlongNormal;
        impulseScalar /= 2.0f; // Divided by 2 because massA(1) + massB(1) = 2

        // Apply impulse forces symmetrically
        vec2s impulse = glms_vec2_scale(normal, impulseScalar);
        *velA         = glms_vec2_add(*velA, impulse);
        *velB         = glms_vec2_sub(*velB, impulse);
    }
}

bool IsOverlappingRect(World *world, int idA, int idB)
{
    CompBody2d *bodyA = &world->body2d[idA];
    CompBody2d *bodyB = &world->body2d[idB];
    vec2s       posA  = (vec2s){world->xforms[idA].pos.x, world->xforms[idA].pos.y};
    vec2s       posB  = (vec2s){world->xforms[idB].pos.x, world->xforms[idB].pos.y};
    vec2s      *velA  = &bodyA->vel;
    vec2s      *velB  = &bodyB->vel;

    // 1. Check for overlap on X and Y axes
    bool overlapX = (posA.x < posB.x + bodyB->dims.x) && (posA.x + bodyA->dims.x > posB.x);
    bool overlapY = (posA.y < posB.y + bodyB->dims.y) && (posA.y + bodyA->dims.y > posB.y);

    if (!overlapX || !overlapY)
        return 0;
    return true;
}

void Grab(vec2s *vel, vec2s pos, vec2s dims)
{
    SolMouse mouse        = Sol_Input_GetMouse();
    vec2s    mPos         = {mouse.x, mouse.y};
    float    min          = fminf(dims.x, dims.y);
    vec2s    center       = (vec2s){pos.x + (dims.x * 0.5f), pos.y + (dims.y * 0.5f)};
    vec2s    scaledCenter = {UISCALE(center.x), UISCALE(center.y)};
    float    dist         = glms_vec2_distance(mPos, scaledCenter);
    if (dist < 0.01f)
        *vel = GLMS_VEC2_ZERO;
    else
        *vel = glms_vec2_scale(glms_vec2_normalize(glms_vec2_sub(mPos, scaledCenter)), dist);
}

void CollideScreenEdge(vec2s *vel, vec2s *pos, vec2s dims)
{
    bool hitX = 0;
    bool hitY = 0;

    if (pos->y < 0)
    {
        pos->y = 0;
        hitY   = 1;
    }
    if (pos->x < 0)
    {
        pos->x = 0;
        hitX   = 1;
    }
    if (pos->y + dims.y > WINDOW_HEIGHT)
    {
        pos->y = WINDOW_HEIGHT - dims.y;
        hitY   = 1;
    }
    if (pos->x + dims.x > WINDOW_WIDTH)
    {
        pos->x = WINDOW_WIDTH - dims.x;
        hitX   = 1;
    }
    if (hitX)
        vel->x *= -1.0f;
    if (hitY)
        vel->y *= -1.0f;
}