#include "s_body2.h"
#include "world.h"
#include "sol_math.h"

bool Sol_Body2_DoesCollide(ScBody2 *body, ScBody2 *bodyB)
{
    return (body->mask << 16) & bodyB->mask;
}

bool IsOverlapping(ScBody2 *bodyA, vec3s posA, ScBody2 *bodyB, vec3s posB)
{
    float maxA_x = posA.x + bodyA->dims.x, maxA_y = posA.y + bodyA->dims.y;
    float maxB_x = posB.x + bodyB->dims.x, maxB_y = posB.y + bodyB->dims.y;

    if (posA.x >= maxB_x || maxA_x <= posB.x || posA.y >= maxB_y || maxA_y <= posB.y)
        return false;
    return true;
}

int Sol_Body2_GetOverlaps(World *world, int id, int *ids, int max_counts)
{
    SparseSet_ScBody2 *set = Sol_Comp_Set(world, ScBody2);
    int dense_idx          = set->sparse[id];
    int count              = set->cnt;
    ScBody2 *bodyA         = &set->data[set->sparse[id]];
    vec3s *positions       = world->xform.pos;
    vec3s posA             = positions[id];

    int hits = 0;
    for (int i = 0; i < count; i++)
    {
        if (i == dense_idx)
            continue;
        int idB        = set->dense[i];
        ScBody2 *bodyB = &set->data[i];
        if (!Sol_Body2_DoesCollide(bodyA, bodyB))
            continue;
        vec3s posB = positions[idB];
        if (IsOverlapping(bodyA, posA, bodyB, posB))
        {
            ids[hits++] = idB;
        }
    }
    return hits;
}

static void Resolve_Rect(ScBody2 *bodyA, vec3s *posA, ScBody2 *bodyB, vec3s *posB)
{
    float invMassA     = bodyA->isStatic ? 0.0f : (bodyA->invMass > 0.0f ? bodyA->invMass : 1.0f);
    float invMassB     = bodyB->isStatic ? 0.0f : (bodyB->invMass > 0.0f ? bodyB->invMass : 1.0f);
    float totalInvMass = invMassA + invMassB;

    if (totalInvMass <= 0.0f)
        return;

    // Load positions directly from pointer targets
    float posA_x = posA->x, posA_y = posA->y;
    float posB_x = posB->x, posB_y = posB->y;

    float maxA_x = posA_x + bodyA->dims.x, maxA_y = posA_y + bodyA->dims.y;
    float maxB_x = posB_x + bodyB->dims.x, maxB_y = posB_y + bodyB->dims.y;

    // AABB Overlap check
    if (posA_x >= maxB_x || maxA_x <= posB_x || posA_y >= maxB_y || maxA_y <= posB_y)
        return;

    // Single-pass Overlap & Normal calculation
    float xOverlap, normX;
    if (posA_x < posB_x)
    {
        xOverlap = maxA_x - posB_x;
        normX    = -1.0f;
    }
    else
    {
        xOverlap = maxB_x - posA_x;
        normX    = 1.0f;
    }

    float yOverlap, normY;
    if (posA_y < posB_y)
    {
        yOverlap = maxA_y - posB_y;
        normY    = -1.0f;
    }
    else
    {
        yOverlap = maxB_y - posA_y;
        normY    = 1.0f;
    }

    // Minimum Translation Vector (MTV)
    float normalX = 0.0f, normalY = 0.0f, penetration;
    if (xOverlap < yOverlap)
    {
        penetration = xOverlap;
        normalX     = normX;
    }
    else
    {
        penetration = yOverlap;
        normalY     = normY;
    }

    // Positional Resolution directly on vec3s pointers
    float invTotal = 1.0f / totalInvMass;
    float ratioA   = invMassA * invTotal;
    float ratioB   = invMassB * invTotal;

    float sepX = normalX * penetration;
    float sepY = normalY * penetration;

    posA->x += sepX * ratioA;
    posA->y += sepY * ratioA;
    posB->x -= sepX * ratioB;
    posB->y -= sepY * ratioB;

    // Velocity Impulse
    float relVelX        = bodyA->vel.x - bodyB->vel.x;
    float relVelY        = bodyA->vel.y - bodyB->vel.y;
    float velAlongNormal = (relVelX * normalX) + (relVelY * normalY);

    if (velAlongNormal >= 0.0f)
        return;

    float restitution = bodyA->restitution < bodyB->restitution ? bodyA->restitution : bodyB->restitution;
    float j           = -(1.0f + restitution) * velAlongNormal * invTotal;

    float impX = normalX * j;
    float impY = normalY * j;

    bodyA->vel.x += impX * invMassA;
    bodyA->vel.y += impY * invMassA;
    bodyB->vel.x -= impX * invMassB;
    bodyB->vel.y -= impY * invMassB;
}

typedef void (*Resolver)(ScBody2 *bodyA, vec3s *posA, ScBody2 *bodyB, vec3s *posB);
const Resolver shape_resolver[SHAPE2_CNT][SHAPE2_CNT] = {
    [SHAPE2_REC][SHAPE2_REC] = Resolve_Rect,
};

const vec3s bounds_min = {0.0f, 0.0f, 0.0f};
const vec3s bounds_max = {WINDOW_WIDTH, WINDOW_HEIGHT, 0.0f};

void Body2_Step(World *world, double dt)
{
    float fdt = (float)dt;
    int i, j;
    SparseSet_ScBody2 *set = Sol_Comp_Set(world, ScBody2);
    int count              = set->cnt;

    // Accum forces
    for (i = 0; i < count; i++)
    {
        ScBody2 *body = &set->data[i];
        body->vel     = glms_vec3_scale(body->vel, 0.95f);
        vec3s accel   = body->vel.y > TERMINAL_VELOCITY ? GLMS_VEC3_ZERO : body->gravity;
        accel         = glms_vec3_add(accel, body->force);
        accel         = glms_vec3_add(accel, body->impulse);
        body->impulse = (vec3s){0};
        body->vel     = vecAdd(body->vel, vecSca(accel, fdt));
    }
    // Apply forces
    for (i = 0; i < count; i++)
    {
        int id               = set->dense[i];
        ScBody2 *body        = &set->data[i];
        world->xform.pos[id] = vecAdd(world->xform.pos[id], vecSca(body->vel, fdt));
    }
    // Gather contacts
    vec3s *positions = world->xform.pos;
    for (i = 0; i < count; i++)
    {
        int id        = set->dense[i];
        ScBody2 *body = &set->data[i];
        if (body->isSensor)
            continue;
        vec3s *posA = &positions[id];

        for (j = i + 1; j < count; j++)
        {
            int idB        = set->dense[j];
            ScBody2 *bodyB = &set->data[j];
            if (bodyB->isSensor || !Sol_Body2_DoesCollide(body, bodyB))
                continue;
            vec3s *posB = &positions[idB];
            if (shape_resolver[body->shape][bodyB->shape])
                shape_resolver[body->shape][bodyB->shape](body, posA, bodyB, posB);
        }
    }
    // Resolve contacts
    for (i = 0; i < count; i++)
    {
        int id        = set->dense[i];
        ScBody2 *body = &set->data[i];
        if (body->ignoreWindow)
            continue;
        Xform xform   = Xform_Get(world, id);
        vec3s old_pos = xform.pos;
        vec3s max_pos = glms_vec3_sub(bounds_max, body->dims);

        // Clamp position
        world->xform.pos[id] = glms_vec3_maxv(bounds_min, glms_vec3_minv(old_pos, max_pos));

        // Edge contact velocity resolution
        if (world->xform.pos[id].x != old_pos.x)
            body->vel.x = -body->vel.x * body->restitution;
        if (world->xform.pos[id].y != old_pos.y)
            body->vel.y = -body->vel.y * body->restitution;
    }
}

bool Sol_Body2_ContainsPoint(World *world, int id, vec2s point)
{
    ScBody2 *body = Sol_Comp_Get(world, id, ScBody2);
    bool overlapX = (point.x > world->xform.pos[id].x) && point.x < (world->xform.pos[id].x + body->dims.x);
    bool overlapY = (point.y > world->xform.pos[id].y) && point.y < (world->xform.pos[id].y + body->dims.y);

    if (overlapX && overlapY)
        return true;

    return false;
}

int Sol_Body2_GetEntAtPoint(World *world, vec2s point)
{
    int best   = -1;
    int zindex = -1;

    SparseSet_ScBody2 *set = Sol_Comp_Set(world, ScBody2);
    for (int i = 0; i < set->cnt; i++)
    {
        int id        = set->dense[i];
        ScBody2 *body = &set->data[i];
        bool overlapX = (point.x > world->xform.pos[id].x) && point.x < (world->xform.pos[id].x + body->dims.x);
        bool overlapY = (point.y > world->xform.pos[id].y) && point.y < (world->xform.pos[id].y + body->dims.y);
        if (overlapX && overlapY && body->zindex > zindex)
        {
            best   = id;
            zindex = body->zindex;
        }
    }

    return best;
}
