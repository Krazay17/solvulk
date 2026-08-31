#include "s_physx.h"
#include "model.h"
#include "sol_math.h"
#include "world.h"

typedef bool (*ShapePairTest)(World *world, int idA, int idB, SolContact *contact);
const ShapePairTest shape_pair_test[SHAPE3_CNT][SHAPE3_CNT] = {
    [SHAPE3_SPH][SHAPE3_SPH] = Collide_Sphere_Sphere,
    [SHAPE3_CAP][SHAPE3_CAP] = Collide_Capsule_Capsule,
    [SHAPE3_CAP][SHAPE3_SPH] = Collide_Capsule_Sphere,
    [SHAPE3_SPH][SHAPE3_CAP] = Collide_Sphere_Capsule,
};
typedef bool (*ShapeTriTest)(World *world, int idA, int idB, const SolTri *tri, SolContact *contact);
const ShapeTriTest shape_tri_test[SHAPE3_CNT] = {
    [SHAPE3_SPH] = Collide_Sphere_Tri,
};

static SolTri worldTris_temp[0xfffff];

static inline BodyData Get_Body_Data(const SolBody3 *body)
{
    // Static fallback: invMass = 0, vel = 0, restitution = 0
    static const BodyData static_body = {0.0f, 0.0f, {{0.0f, 0.0f, 0.0f}}};
    return body ? (BodyData){body->invMass, body->restitution, body->vel} : static_body;
}

void Build_Tables(World *world, SysPhysx *sys, float fdt)
{
    SpatialTable_Clear(&sys->dynamic_table);
    SpatialTable_Clear(&sys->dynamic_tri_table);

    SparseSet_SolBody3 *bodySet = Sol_Comp_Set(world, SolBody3);
    for (int i = 0; i < bodySet->cnt; i++)
    {
        int       id      = bodySet->dense[i];
        SolBody3 *body    = &bodySet->data[i];
        SolXform *xform   = Sol_Comp_Get(world, id, SolXform);
        vec3s     prevPos = xform->pos;
        vec3s     nextPos = vecAdd(prevPos, (vecSca(body->vel, fdt)));
        vec3s     min     = vecSub(glms_vec3_minv(prevPos, nextPos), body->dims);
        vec3s     max     = vecAdd(glms_vec3_maxv(prevPos, nextPos), body->dims);
        Spatial_FloorHashInsert(&sys->dynamic_table, min, max, id);
    }

    SparseSet_SolMeshCollider *meshSet = Sol_Comp_Set(world, SolMeshCollider);
    for (int i = 0; i < meshSet->cnt; i++)
    {
        int              id    = meshSet->dense[i];
        SolMeshCollider *mesh  = &meshSet->data[i];
        SolXform        *xform = Sol_Comp_Get(world, id, SolXform);
        SolModel        *model = Sol_Comp_Get(world, id, SolModel);
        if (!mesh->isDirty || !model || !xform)
            continue;
        int     tricount = loaded_models[model->kind].tri_count;
        SolTri *tris     = loaded_models[model->kind].tris;
        for (int t = 0; t < tricount; t++)
        {
            SolTri worldTri = SolTri_GetWorldSpace(&tris[t], xform->rot, xform->pos, xform->sca);

            vec3s min = {
                .x = fminf(worldTri.a.x, fminf(worldTri.b.x, worldTri.c.x)),
                .y = fminf(worldTri.a.y, fminf(worldTri.b.y, worldTri.c.y)),
                .z = fminf(worldTri.a.z, fminf(worldTri.b.z, worldTri.c.z)),
            };
            vec3s max = {
                .x = fmaxf(worldTri.a.x, fmaxf(worldTri.b.x, worldTri.c.x)),
                .y = fmaxf(worldTri.a.y, fmaxf(worldTri.b.y, worldTri.c.y)),
                .z = fmaxf(worldTri.a.z, fmaxf(worldTri.b.z, worldTri.c.z)),
            };

            Spatial_FloorHashInsert(&sys->dynamic_tri_table, min, max, MAKE_TRI_VALUE(id, t));
        }
        mesh->isDirty = false;
    }

    SparseSet_SolStage *stageSet = Sol_Comp_Set(world, SolStage);
    for (int i = 0; i < stageSet->cnt; i++)
    {
        int       id    = stageSet->dense[i];
        SolStage *stage = &stageSet->data[i];
        SolModel *model = Sol_Comp_Get(world, id, SolModel);
        SolXform *xform = Sol_Comp_Get(world, id, SolXform);
        if (!stage->isDirty || !model || !xform)
            continue;
        int     tricount = loaded_models[model->kind].tri_count;
        SolTri *tris     = loaded_models[model->kind].tris;
        for (int t = 0; t < tricount; t++)
        {
            sys->worldTris[t] = SolTri_GetWorldSpace(&tris[t], xform->rot, xform->pos, xform->sca);
        }
        SpatialGrid_BuildFromTris(&sys->static_tri_grid, sys->worldTris, tricount, id);
        stage->isDirty = false;
    }
}

void Resolve_Contact(SolBody3 *bodyA, SolXform *xformA, SolBody3 *bodyB, SolXform *xformB, SolContact *contact)
{
    // Default static fallback values if body component is missing
    float invMassA = bodyA ? bodyA->invMass : 0.0f;
    float invMassB = bodyB ? bodyB->invMass : 0.0f;

    float totalInvMass = invMassA + invMassB;
    if (totalInvMass <= 0.0f)
        return; // Two static/kinematic objects, no resolution needed

    vec3s velA = bodyA ? bodyA->vel : GLMS_VEC3_ZERO;
    vec3s velB = bodyB ? bodyB->vel : GLMS_VEC3_ZERO;

    // --- Positional Correction ---
    const float slack   = 0.01f;
    const float percent = 0.2f;

    float corrMag    = (fmaxf(contact->penetration - slack, 0.0f) / totalInvMass) * percent;
    vec3s correction = glms_vec3_scale(contact->normal, corrMag);

    if (xformA && invMassA > 0.0f)
        xformA->pos = glms_vec3_add(xformA->pos, glms_vec3_scale(correction, invMassA));

    if (xformB && invMassB > 0.0f)
        xformB->pos = glms_vec3_sub(xformB->pos, glms_vec3_scale(correction, invMassB));

    // --- Velocity Impulse ---
    vec3s relativeVel    = glms_vec3_sub(velA, velB);
    float velAlongNormal = glms_vec3_dot(relativeVel, contact->normal);

    if (velAlongNormal > 0.0f)
        return;

    float restA = bodyA ? bodyA->restitution : 0.0f;
    float restB = bodyB ? bodyB->restitution : 0.0f;
    float e     = fminf(restA, restB);

    float j       = -(1.0f + e) * velAlongNormal / totalInvMass;
    vec3s impulse = glms_vec3_scale(contact->normal, j);

    if (bodyA && invMassA > 0.0f)
        bodyA->vel = glms_vec3_add(bodyA->vel, glms_vec3_scale(impulse, invMassA));

    if (bodyB && invMassB > 0.0f)
        bodyB->vel = glms_vec3_sub(bodyB->vel, glms_vec3_scale(impulse, invMassB));
}

void Collisions_Static_Stage(World *world, int idA, SysPhysx *sys, SolXform *xform, SolBody3 *body, float fdt)
{
    if (sys->static_tri_grid.item_count == 0)
        return;
    vec3s prevPos = xform->pos;
    vec3s nextPos = vecAdd(prevPos, vecSca(body->vel, fdt));

    // Calculate swept AABB bounds using body dimensions
    vec3s min = vecSub(glms_vec3_minv(prevPos, nextPos), body->dims);
    vec3s max = vecAdd(glms_vec3_maxv(prevPos, nextPos), body->dims);

    // Convert world bounds to grid cell ranges
    ivec3s minCell = SpatialGrid_WorldToCell(&sys->static_tri_grid, min);
    ivec3s maxCell = SpatialGrid_WorldToCell(&sys->static_tri_grid, max);

    // Iterate over all cells overlapping the entity's swept volume
    for (int z = minCell.z; z <= maxCell.z; z++)
    {
        for (int y = minCell.y; y <= maxCell.y; y++)
        {
            for (int x = minCell.x; x <= maxCell.x; x++)
            {
                uint32_t cellIdx = SpatialGrid_GetCellIndex(&sys->static_tri_grid, x, y, z);
                GridCell cell    = sys->static_tri_grid.cells[cellIdx];

                // Check all triangles residing in this cell
                for (uint32_t i = 0; i < cell.count; i++)
                {
                    uint32_t packedData = sys->static_tri_grid.index_buffer[cell.offset + i];

                    // Unpack entity ID and triangle index from bit-packed u32
                    uint32_t entityId, triIndex;
                    Unpack_StageTri(packedData, &entityId, &triIndex);

                    const SolTri *tri = &sys->worldTris[triIndex];

                    SolContact contact;
                    if (shape_tri_test[body->shape](world, idA, entityId, tri, &contact))
                    {
                        if (sys->contacts.contact_cnt < MAX_CONTACTS - 1)
                        {
                            int idx                        = sys->contacts.contact_cnt++;
                            sys->contacts.contact[idx]     = contact;
                            sys->contacts.contact[idx].id  = idA;
                            sys->contacts.contact[idx].idB = entityId;
                        }
                    }
                }
            }
        }
    }
}

void Collisions_Dynamic_Bodies(World *world, int idA, SysPhysx *sys, SolXform *xform, SolBody3 *body)
{
    SpatialCell cell = Spatial_Cell_GetNeighbors(xform->pos, sys->dynamic_table.cellSize);

    for (int n = 0; n < 27; n++)
    {
        u32 cellHash = cell.neighborHashes[n];
        u32 entry    = SpatialTable_GetEntry(&sys->dynamic_table, cellHash);

        while (entry != SPATIAL_NULL)
        {
            int idB = (int)sys->dynamic_table.value[entry];

            if (idA < idB) // Deduplicate pairs
            {
                SolBody3 *other_body = Sol_Comp_Get(world, idB, SolBody3);

                // Don't collide two immovable bodies
                if ((body->mass > 0.0f || other_body->mass > 0.0f) && Sol_Physx_DoesCollide(body, other_body))
                {
                    SolContact contact;
                    if (shape_pair_test[body->shape][other_body->shape] &&
                        shape_pair_test[body->shape][other_body->shape](world, idA, idB, &contact))
                    {
                        if (sys->contacts.contact_cnt < MAX_CONTACTS - 1)
                        {
                            int idx                        = sys->contacts.contact_cnt++;
                            sys->contacts.contact[idx]     = contact;
                            sys->contacts.contact[idx].id  = idA;
                            sys->contacts.contact[idx].idB = idB;
                        }
                    }
                }
            }
            entry = sys->dynamic_table.next[entry];
        }
    }
}

void Collisions_Dynamic_Tris(World *world, int idA, SysPhysx *sys, SolXform *xform, SolBody3 *body)
{
    SpatialCell cell = Spatial_Cell_GetNeighbors(xform->pos, sys->dynamic_tri_table.cellSize);
    for (int n = 0; n < 27; n++)
    {
        u32 cellHash = cell.neighborHashes[n];
        u32 entry    = SpatialTable_GetEntry(&sys->dynamic_tri_table, cellHash);

        while (entry != SPATIAL_NULL)
        {
            u32 val        = sys->dynamic_tri_table.value[entry];
            int modelEntID = GET_TRI_ENTITY(val);

            if (idA != modelEntID && shape_tri_test[body->shape])
            {
                int       triIdx = GET_TRI_INDEX(val);
                SolModel *model  = Sol_Comp_Get(world, modelEntID, SolModel);
                SolXform *xformB = Sol_Comp_Get(world, modelEntID, SolXform);
                if (model && xformB)
                {
                    SolTri localTri = loaded_models[model->kind].tris[triIdx];
                    SolTri worldTri = SolTri_GetWorldSpace(&localTri, xformB->rot, xformB->pos, xformB->sca);

                    SolContact contact;
                    if (shape_tri_test[body->shape](world, idA, modelEntID, &worldTri, &contact))
                    {
                        if (sys->contacts.contact_cnt < MAX_CONTACTS - 1)
                        {
                            int idx                        = sys->contacts.contact_cnt++;
                            sys->contacts.contact[idx]     = contact;
                            sys->contacts.contact[idx].id  = idA;
                            sys->contacts.contact[idx].idB = modelEntID;
                        }
                    }
                }
            }
            entry = sys->dynamic_tri_table.next[entry];
        }
    }
}

void Collisions_Static_Tris(World *world, int idA, SysPhysx *sys, SolXform *xform, SolBody3 *body)
{
    SpatialCell cell = Spatial_Cell_GetNeighbors(xform->pos, sys->static_tri_table.cellSize);
#define MAX_TESTED_TRIS 128
    int testedTris[MAX_TESTED_TRIS];
    int testedCount = 0;
    for (int n = 0; n < 27; n++)
    {
        u32 cellHash = cell.neighborHashes[n];
        u32 entry    = SpatialTable_GetEntry(&sys->static_tri_table, cellHash);

        while (entry != SPATIAL_NULL)
        {
            u32 val        = sys->static_tri_table.value[entry];
            int modelEntID = GET_TRI_ENTITY(val);

            if (idA != modelEntID && shape_tri_test[body->shape])
            {
                int  triIdx        = GET_TRI_INDEX(val);
                bool alreadyTested = false;
                for (int t = 0; t < testedCount; t++)
                {
                    if (testedTris[t] == triIdx)
                    {
                        alreadyTested = true;
                        break;
                    }
                }
                if (!alreadyTested)
                {
                    SolModel *model  = Sol_Comp_Get(world, modelEntID, SolModel);
                    SolXform *xformB = Sol_Comp_Get(world, modelEntID, SolXform);
                    if (model && xformB)
                    {
                        SolTri localTri = loaded_models[model->kind].tris[triIdx];
                        SolTri worldTri = SolTri_GetWorldSpace(&localTri, xformB->rot, xformB->pos, xformB->sca);

                        SolContact contact;
                        if (shape_tri_test[body->shape](world, idA, modelEntID, &worldTri, &contact))
                        {
                            if (sys->contacts.contact_cnt < MAX_CONTACTS - 1)
                            {
                                int idx                        = sys->contacts.contact_cnt++;
                                sys->contacts.contact[idx]     = contact;
                                sys->contacts.contact[idx].id  = idA;
                                sys->contacts.contact[idx].idB = modelEntID;
                            }
                        }
                    }
                }
            }
            entry = sys->static_tri_table.next[entry];
        }
    }
}

bool Collide_Sphere_Sphere(World *world, int idA, int idB, SolContact *contact)
{
    SolXform *xform  = Sol_Comp_Get(world, idA, SolXform);
    SolXform *xformB = Sol_Comp_Get(world, idB, SolXform);
    SolBody3 *body3  = Sol_Comp_Get(world, idA, SolBody3);
    SolBody3 *body3B = Sol_Comp_Get(world, idB, SolBody3);

    vec3s delta     = glms_vec3_sub(xform->pos, xformB->pos);
    float distSq    = glms_vec3_dot(delta, delta);
    float radiusSum = body3->dims.x + body3B->dims.x;

    if (distSq >= (radiusSum * radiusSum) || distSq < 0.0001f)
        return false;

    float distance = sqrtf(distSq);

    contact->normal      = glms_vec3_scale(delta, 1.0f / distance);
    contact->penetration = radiusSum - distance;
    contact->pos         = glms_vec3_add(xformB->pos, glms_vec3_scale(contact->normal, contact->penetration));

    return true;
}

bool Collide_Capsule_Capsule(World *world, int idA, int idB, SolContact *hit)
{
    return false;
}
bool Collide_Sphere_Capsule(World *world, int idA, int idB, SolContact *hit)
{
    return false;
}
bool Collide_Capsule_Sphere(World *world, int idA, int idB, SolContact *hit)
{
    return false;
}
bool Collide_Box_Box(World *world, int idA, int idB, SolContact *hit)
{
    return false;
}
bool Collide_Box_Sphere(World *world, int idA, int idB, SolContact *hit)
{
    return false;
}
bool Collide_Sphere_Box(World *world, int idA, int idB, SolContact *hit)
{
    return false;
}

bool Collide_Sphere_Tri(World *world, int idA, int idB, const SolTri *tri, SolContact *contact)
{
    SolXform *xform = Sol_Comp_Get(world, idA, SolXform);
    SolBody3 *body3 = Sol_Comp_Get(world, idA, SolBody3);

    vec3s closestP = ClosestPointOnTriangle(xform->pos, tri->a, tri->b, tri->c);
    vec3s delta    = glms_vec3_sub(xform->pos, closestP);
    float distSq   = glms_vec3_dot(delta, delta);

    if (distSq >= body3->dims.x * body3->dims.x)
        return false;

    float dist        = sqrtf(distSq);
    vec3s normal      = dist > 0.0001f ? glms_vec3_scale(delta, 1.0f / dist) : tri->normal;
    float penetration = body3->dims.x - dist;

    contact->pos         = closestP;
    contact->penetration = penetration;
    contact->normal      = normal;

    return true;
}

bool Collide_Capsule_Tri(World *world, int idA, int idB, SolContact *hit)
{
    return false;
}

bool SphereCast_VsSphere(vec3s rayPos, vec3s rayDir, float rayLen, float castRadius, vec3s spherePos,
                         float sphereRadius, SolRayResult *hit)
{
    float combinedR = castRadius + sphereRadius;

    vec3s m = glms_vec3_sub(rayPos, spherePos);
    float b = glms_vec3_dot(m, rayDir);
    float c = glms_vec3_dot(m, m) - combinedR * combinedR;

    // Ray starts already inside? Hit at start.
    if (c < 0.0f)
    {
        hit->pos  = rayPos;
        hit->dist = 0.0f;
        hit->norm = glms_vec3_normalize(glms_vec3_sub(rayPos, spherePos));
        return true;
    }

    // Ray pointing away from sphere
    if (b > 0.0f)
        return false;

    float discr = b * b - c;
    if (discr < 0.0f)
        return false;

    float t = -b - sqrtf(discr);
    if (t > rayLen)
        return false;
    if (t < 0.0f)
        t = 0.0f;

    hit->dist        = t;
    vec3s castCenter = glms_vec3_add(rayPos, glms_vec3_scale(rayDir, t));
    hit->pos         = castCenter; // could refine to surface
    hit->norm        = glms_vec3_normalize(glms_vec3_sub(castCenter, spherePos));
    return true;
}

bool SphereCast_VsCapsule(vec3s rayPos, vec3s rayDir, float rayLen, float castRadius, vec3s capPos, float capRadius,
                          float capHeight, SolRayResult *hit)
{
    float combinedR = castRadius + capRadius;
    float halfSpan  = capHeight * 0.5f - capRadius;
    if (halfSpan < 0)
        halfSpan = 0;

    vec3s spineTop    = {{capPos.x, capPos.y + halfSpan, capPos.z}};
    vec3s spineBottom = {{capPos.x, capPos.y - halfSpan, capPos.z}};
    vec3s rayEnd      = glms_vec3_add(rayPos, glms_vec3_scale(rayDir, rayLen));

    vec3s closestRay, closestSpine;
    Closest_Points_Segment_Segment(rayPos, rayEnd, spineBottom, spineTop, &closestRay, &closestSpine);

    vec3s delta  = glms_vec3_sub(closestRay, closestSpine);
    float distSq = glms_vec3_dot(delta, delta);

    if (distSq >= combinedR * combinedR)
        return false;

    // Approximate hit distance as distance from rayPos to closestRay
    float t = glms_vec3_norm(glms_vec3_sub(closestRay, rayPos));

    hit->dist = t;
    hit->pos  = closestSpine;
    hit->norm = (distSq > 0.0001f) ? glms_vec3_normalize(delta) : (vec3s){{0, 1, 0}};
    return true;
}

static inline bool SweptSphere_Tri_Test(vec3s start, vec3s dir, float radius, const SolTri *tri, SolSweptHit *outHit)
{
    vec3s edge1 = glms_vec3_sub(tri->b, tri->a);
    vec3s edge2 = glms_vec3_sub(tri->c, tri->a);
    vec3s N     = glms_vec3_normalize(glms_vec3_cross(edge1, edge2));

    // Distance from sphere start center to plane
    float distToPlane = glms_vec3_dot(glms_vec3_sub(start, tri->a), N);
    float denom       = glms_vec3_dot(dir, N);

    // Moving parallel to or away from the front face
    if (denom >= -1e-6f)
        return false;

    // Time when sphere surface contacts plane: (dist - radius) / -denom
    float t_plane = (distToPlane - radius) / -denom;

    if (t_plane < 0.0f || t_plane > 1.0f)
        return false;

    // Projected impact point on plane
    vec3s planeHit = glms_vec3_add(start, glms_vec3_scale(dir, t_plane));
    vec3s contactP = glms_vec3_sub(planeHit, glms_vec3_scale(N, radius));

    // --- Barycentric Test on Plane Contact Point ---
    vec3s v0 = glms_vec3_sub(tri->c, tri->a);
    vec3s v1 = glms_vec3_sub(tri->b, tri->a);
    vec3s v2 = glms_vec3_sub(contactP, tri->a);

    float dot00 = glms_vec3_dot(v0, v0);
    float dot01 = glms_vec3_dot(v0, v1);
    float dot02 = glms_vec3_dot(v0, v2);
    float dot11 = glms_vec3_dot(v1, v1);
    float dot12 = glms_vec3_dot(v1, v2);

    float invDenom = 1.0f / (dot00 * dot11 - dot01 * dot01);
    float u        = (dot11 * dot02 - dot01 * dot12) * invDenom;
    float v        = (dot00 * dot12 - dot01 * dot02) * invDenom;

    // Hit inside triangle face
    if (u >= 0.0f && v >= 0.0f && (u + v) <= 1.0f)
    {
        outHit->hit    = true;
        outHit->t      = t_plane;
        outHit->point  = contactP;
        outHit->normal = N;
        return true;
    }

    return false;
}

bool Collisions_Swept_Static_Tris(World *world, int idA, SysPhysx *sys, SolXform *xform, SolBody3 *body, float dt,
                                  SolSweptHit *earliestHit)
{
    vec3s moveDir = glms_vec3_scale(body->vel, dt);
    float moveLen = glms_vec3_norm(moveDir);

    // Fast-path: Skip swept test if object is stationary or moving very slowly
    if (moveLen < 0.001f)
        return false;

    float radius = fmaxf(body->dims.x, fmaxf(body->dims.y, body->dims.z)) * 0.5f;

    // 1. Calculate Swept AABB
    vec3s startPos = xform->pos;
    vec3s endPos   = glms_vec3_add(startPos, moveDir);

    vec3s minBounds = glms_vec3_sub(glms_vec3_minv(startPos, endPos), (vec3s){radius, radius, radius});
    vec3s maxBounds = glms_vec3_add(glms_vec3_maxv(startPos, endPos), (vec3s){radius, radius, radius});

    // 2. Query spatial hash cells across the swept bounds
    float invCell = sys->static_tri_table.invCellSize;
    int   mask    = sys->static_tri_table.size - 1;
    int   x0      = (int)floorf(minBounds.x * invCell);
    int   x1      = (int)floorf(maxBounds.x * invCell);
    int   y0      = (int)floorf(minBounds.y * invCell);
    int   y1      = (int)floorf(maxBounds.y * invCell);
    int   z0      = (int)floorf(minBounds.z * invCell);
    int   z1      = (int)floorf(maxBounds.z * invCell);

    earliestHit->hit = false;
    earliestHit->t   = 1.0f;

#define MAX_SWEPT_TRIS 256
    int testedTris[MAX_SWEPT_TRIS];
    int testedCount = 0;

    for (int x = x0; x <= x1; x++)
    {
        for (int y = y0; y <= y1; y++)
        {
            for (int z = z0; z <= z1; z++)
            {
                u32 bucket = hash_coords(x, y, z) & mask;
                u32 entry  = sys->static_tri_table.head[bucket];

                while (entry != SPATIAL_NULL)
                {
                    u32 val        = sys->static_tri_table.value[entry];
                    int modelEntID = GET_TRI_ENTITY(val);
                    int triIdx     = GET_TRI_INDEX(val);

                    // Deduplicate tested triangles
                    bool tested = false;
                    for (int t = 0; t < testedCount; t++)
                    {
                        if (testedTris[t] == triIdx)
                        {
                            tested = true;
                            break;
                        }
                    }

                    if (!tested && testedCount < MAX_SWEPT_TRIS)
                    {
                        testedTris[testedCount++] = triIdx;

                        SolModel *model  = Sol_Comp_Get(world, modelEntID, SolModel);
                        SolXform *xformB = Sol_Comp_Get(world, modelEntID, SolXform);

                        if (model && xformB)
                        {
                            SolTri localTri = loaded_models[model->kind].tris[triIdx];
                            SolTri worldTri = SolTri_GetWorldSpace(&localTri, xformB->rot, xformB->pos, xformB->sca);

                            SolSweptHit hit;
                            if (SweptSphere_Tri_Test(startPos, moveDir, radius, &worldTri, &hit))
                            {
                                if (hit.t < earliestHit->t)
                                {
                                    *earliestHit = hit;
                                }
                            }
                        }
                    }
                    entry = sys->static_tri_table.next[entry];
                }
            }
        }
    }

    return earliestHit->hit;
}