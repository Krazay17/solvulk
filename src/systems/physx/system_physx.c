#include <cglm/struct.h>
#include <omp.h>

#include "sol_core.h"

static int bodyIds[MAX_ENTS];
static SpatialCell cells[MAX_ENTS];

static SolProfiler profSpatialStatic = {.name = "Spatial Static"};
static SolProfiler profBuildDynamic = {.name = "Build Dynamic"};
static SolProfiler profSpatialDynamicPos = {.name = "Spatial Dynamic Pos"};
static SolProfiler profSpatialDynamicVel = {.name = "Spatial Dynamic Vel"};
static int profFrame = 0;

void Sol_System_Step_Physx_3d(World *world, double dt, double time)
{
    float fdt = (float)dt;
    int required = HAS_BODY3 | HAS_XFORM;
    int i, j, k;
    int count = world->activeCount;
    WorldSpatial *ws = &world->worldSpatial;

    Prof_Begin(&profSpatialStatic);
    // Static position resolution
#pragma omp parallel for if (count > 500) schedule(dynamic, 16)
    for (i = 0; i < count; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;
        CompXform *xform = &world->xforms[id];
        CompBody *body = &world->bodies[id];

        body->grounded = 0;

        vec3s accel = SOL_PHYS_GRAV;
        accel = glms_vec3_add(accel, body->force);
        accel = glms_vec3_add(accel, body->impulse);
        body->impulse = (vec3s){0};
        body->vel = glms_vec3_add(body->vel, glms_vec3_scale(accel, fdt));

        int totalChecks = 0;

        // Per-entity substep count based on speed
        float speed = glms_vec3_norm(body->vel);
        float stepDist = body->radius * 0.9f;
        int substeps = (int)ceilf(speed * fdt / stepDist);
        if (substeps < 1)
            substeps = 1;
        if (substeps > 8)
            substeps = 8;
        float subDt = fdt / substeps;
        for (int s = 0; s < substeps; s++)
        {
            xform->pos = glms_vec3_add(xform->pos, glms_vec3_scale(body->vel, subDt));
            SpatialCell cell = GetSpatialCell(xform->pos);
            for (u8 n = 0; n < 27; n++)
            {
                u32 entry = ws->staticWorld.head[cell.neighborHashes[n] & ws->staticWorld.mask];
                while (entry != SPATIAL_NULL)
                {
                    totalChecks++;
                    CollisionTri *tri = &ws->tris[ws->staticWorld.value[entry]];
                    SolCollision col = ResolveSphereTriangle(body, &xform->pos, tri);
                    if (col.didCollide && col.normal.y > 0.5f)
                        body->grounded = 1;
                    entry = ws->staticWorld.next[entry];
                }
            }
        }
        if (id == world->playerID)
            Sol_Debug_Add("TriChecks", totalChecks);
    }
    Prof_End(&profSpatialStatic);

    Prof_Begin(&profBuildDynamic);
    Sol_Spatial_Build_Dynamic(world);
    Prof_End(&profBuildDynamic);

    Prof_Begin(&profSpatialDynamicPos);
    // 2. Dynamic position resolution
#pragma omp parallel for if (count > 500) schedule(dynamic, 16)
    for (j = 0; j < count; j++)
    {
        int id = world->activeEntities[j];
        if ((world->masks[id] & required) != required)
            continue;
        CompBody *body = &world->bodies[id];
        CompXform *xform = &world->xforms[id];
        if (body->invMass <= 0.0f)
            continue;

        SpatialCell cell = GetSpatialCell(xform->pos);
        for (u8 n = 0; n < 27; n++)
        {
            u32 entry = ws->dynamicUnits.head[cell.neighborHashes[n] & ws->dynamicUnits.mask];
            while (entry != SPATIAL_NULL)
            {
                u32 otherID = ws->dynamicUnits.value[entry];
                if (id < (int)otherID)
                    ResolvePositionOnly(body, xform,
                                        &world->bodies[otherID],
                                        &world->xforms[otherID]);
                entry = ws->dynamicUnits.next[entry];
            }
        }
    }
    Prof_End(&profSpatialDynamicPos);

    Prof_Begin(&profSpatialDynamicVel);
    // 3. Dynamic velocity resolution
#pragma omp parallel for if (count > 500) schedule(dynamic, 16)
    for (k = 0; k < count; k++)
    {
        int id = world->activeEntities[k];
        if ((world->masks[id] & required) != required)
            continue;
        CompBody *body = &world->bodies[id];
        CompXform *xform = &world->xforms[id];
        if (body->invMass <= 0.0f)
            continue;

        SpatialCell cell = GetSpatialCell(xform->pos);
        for (u8 n = 0; n < 27; n++)
        {
            u32 entry = ws->dynamicUnits.head[cell.neighborHashes[n] & ws->dynamicUnits.mask];
            while (entry != SPATIAL_NULL)
            {
                u32 otherID = ws->dynamicUnits.value[entry];
                if (id < (int)otherID)
                    ResolveVelocityOnly(body, xform,
                                        &world->bodies[otherID],
                                        &world->xforms[otherID]);
                entry = ws->dynamicUnits.next[entry];
            }
        }
    }
    Prof_End(&profSpatialDynamicVel);

    profFrame++;
    if (profFrame % 300 == 0)
    {
        Prof_Print(&profSpatialStatic);
        Prof_Print(&profBuildDynamic);
        Prof_Print(&profSpatialDynamicPos);
        Prof_Print(&profSpatialDynamicVel);
    }
}

void Sol_System_Step_Physx_2d(World *world, double dt, double time)
{
    float fdt = (float)dt;
    int required = HAS_BODY2 | HAS_XFORM;
    for (int i = 0; i < world->activeCount; ++i)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) == required)
        {
            CompBody *body = &world->bodies[id];
            CompMovement *movement = &world->movements[id];
            CompXform *xform = &world->xforms[id];

            float moveGrav = MOVE_STATE_FORCES[movement->configId][movement->moveState].gravity;
            body->vel.y = moveGrav ? body->vel.y + moveGrav * fdt
                                   : body->vel.y + SOL_PHYS_GRAV.y * fdt;
            vec3s finalVel = glms_vec3_scale(body->vel, fdt * 20.0f);
            xform->pos = glms_vec3_add(xform->pos, finalVel);

            if (xform->pos.y + body->height >= Sol_GetState()->windowHeight)
            {
                xform->pos.y = Sol_GetState()->windowHeight - body->height;
                body->vel.y = 0;
            }
        }
    }
}

void ResolvePositionOnly(CompBody *aBody, CompXform *aXform,
                         CompBody *bBody, CompXform *bXform)
{
    vec3s delta = glms_vec3_sub(aXform->pos, bXform->pos);
    float combined = aBody->radius + bBody->radius;
    float distSq = glms_vec3_dot(delta, delta);

    if (distSq >= combined * combined || distSq <= 0.0f)
        return;

    float dist = sqrtf(distSq);
    vec3s normal = glms_vec3_scale(delta, 1.0f / dist);
    float penetration = combined - dist;
    float totalInvMass = aBody->invMass + bBody->invMass;
    if (totalInvMass <= 0.0f)
        return;

    float correction = fmaxf(penetration - SOL_PHYS_COLLISION_SKIN, 0.0f) / totalInvMass;
    vec3s corrVec = glms_vec3_scale(normal, correction);

    aXform->pos = glms_vec3_add(aXform->pos, glms_vec3_scale(corrVec, aBody->invMass));
    bXform->pos = glms_vec3_sub(bXform->pos, glms_vec3_scale(corrVec, bBody->invMass));
}

void ResolveVelocityOnly(CompBody *aBody, CompXform *aXform,
                         CompBody *bBody, CompXform *bXform)
{
    vec3s delta = glms_vec3_sub(aXform->pos, bXform->pos);
    float combined = aBody->radius + bBody->radius;
    float distSq = glms_vec3_dot(delta, delta);

    // Small margin so we catch pairs that are touching after position solve
    float margin = combined + SOL_PHYS_COLLISION_SKIN;
    if (distSq >= margin * margin || distSq <= 0.0f)
        return;

    float dist = sqrtf(distSq);
    vec3s normal = glms_vec3_scale(delta, 1.0f / dist);

    vec3s relVel = glms_vec3_sub(aBody->vel, bBody->vel);
    float velAlongNormal = glms_vec3_dot(relVel, normal);
    if (velAlongNormal > 0)
        return;

    float totalInvMass = aBody->invMass + bBody->invMass;
    if (totalInvMass <= 0.0f)
        return;

    float e = fminf(aBody->restitution, bBody->restitution);
    float j = -(1.0f + e) * velAlongNormal / totalInvMass;
    vec3s impulse = glms_vec3_scale(normal, j);

    aBody->vel = glms_vec3_add(aBody->vel, glms_vec3_scale(impulse, aBody->invMass));
    bBody->vel = glms_vec3_sub(bBody->vel, glms_vec3_scale(impulse, bBody->invMass));
}

// Fill the worlds dynamic spatial table
void Sol_Spatial_Build_Dynamic(World *world)
{
    SpatialTable *dynamic = &world->worldSpatial.dynamicUnits;
    SpatialTable_Clear(dynamic);

    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if (!(world->masks[id] & HAS_BODY3))
            continue;

        vec3s pos = world->xforms[id].pos;
        int ix = (int)floorf(pos.x / SPATIAL_CELL_SIZE);
        int iy = (int)floorf(pos.y / SPATIAL_CELL_SIZE);
        int iz = (int)floorf(pos.z / SPATIAL_CELL_SIZE);
        u32 hash = HashCoords(ix, iy, iz, dynamic->mask);
        SpatialTable_Insert(dynamic, hash, (u32)id);
    }
}

void Sol_Spatial_AddStatic(World *world, SolModel *model, CompXform *xform)
{
    WorldSpatial *ws = &world->worldSpatial;

    u32 totalTris = 0;
    for (u32 m = 0; m < model->meshCount; m++)
        totalTris += model->meshes[m].indexCount / 3;

    u32 oldCount = ws->triCount;
    totalTris += oldCount;

    ws->tris = realloc(ws->tris, sizeof(CollisionTri) * totalTris);
    ws->triCount = totalTris;

    mat3s mat = glms_quat_mat3(xform->quat);

    u32 triIdx = oldCount;
    for (u32 m = 0; m < model->meshCount; m++)
    {
        SolMesh *mesh = &model->meshes[m];
        for (u32 i = 0; i < mesh->indexCount; i += 3)
        {
            CollisionTri *t = &ws->tris[triIdx];

            vec3s verts[3] = {
                *(vec3s *)model->vertices[mesh->vertexOffset + model->indices[mesh->indexOffset + i + 0]].position,
                *(vec3s *)model->vertices[mesh->vertexOffset + model->indices[mesh->indexOffset + i + 1]].position,
                *(vec3s *)model->vertices[mesh->vertexOffset + model->indices[mesh->indexOffset + i + 2]].position,
            };

            for (int v = 0; v < 3; v++)
            {
                verts[v] = glms_vec3_mul(verts[v], xform->scale);
                verts[v] = glms_mat3_mulv(mat, verts[v]);
                verts[v] = glms_vec3_add(verts[v], xform->pos);
            }

            t->a = verts[0];
            t->b = verts[1];
            t->c = verts[2];

            t->center = glms_vec3_scale(
                glms_vec3_add(glms_vec3_add(t->a, t->b), t->c), 1.0f / 3.0f);
            float da = glms_vec3_norm(glms_vec3_sub(t->a, t->center));
            float db = glms_vec3_norm(glms_vec3_sub(t->b, t->center));
            float dc = glms_vec3_norm(glms_vec3_sub(t->c, t->center));
            t->boundRadius = fmaxf(da, fmaxf(db, dc));

            vec3s edge1 = glms_vec3_sub(t->b, t->a);
            vec3s edge2 = glms_vec3_sub(t->c, t->a);
            vec3s cross = glms_vec3_cross(edge1, edge2);
            float len = glms_vec3_norm(cross);
            if (len > 0.00001f)
                t->normal = glms_vec3_scale(cross, 1.0f / len);
            else
                t->normal = (vec3s){0, 1, 0}; // Default up normal for broken tris

            float minX = fminf(t->a.x, fminf(t->b.x, t->c.x));
            float maxX = fmaxf(t->a.x, fmaxf(t->b.x, t->c.x));
            float minY = fminf(t->a.y, fminf(t->b.y, t->c.y));
            float maxY = fmaxf(t->a.y, fmaxf(t->b.y, t->c.y));
            float minZ = fminf(t->a.z, fminf(t->b.z, t->c.z));
            float maxZ = fmaxf(t->a.z, fmaxf(t->b.z, t->c.z));

            int x0 = (int)floorf(minX / SPATIAL_CELL_SIZE);
            int x1 = (int)floorf(maxX / SPATIAL_CELL_SIZE);
            int y0 = (int)floorf(minY / SPATIAL_CELL_SIZE);
            int y1 = (int)floorf(maxY / SPATIAL_CELL_SIZE);
            int z0 = (int)floorf(minZ / SPATIAL_CELL_SIZE);
            int z1 = (int)floorf(maxZ / SPATIAL_CELL_SIZE);

            for (int x = x0; x <= x1; x++)
                for (int y = y0; y <= y1; y++)
                    for (int z = z0; z <= z1; z++)
                        SpatialTable_Insert(&ws->staticWorld, HashCoords(x, y, z, ws->staticWorld.mask), triIdx);

            triIdx++;
        }
    }
    printf("Static world: %u tris, %u spatial entries\n", totalTris, ws->staticWorld.count);
}

SpatialCell GetSpatialCell(vec3s pos)
{
    SpatialCell cell;
    cell.ix = (int)floorf(pos.x / SPATIAL_CELL_SIZE);
    cell.iy = (int)floorf(pos.y / SPATIAL_CELL_SIZE);
    cell.iz = (int)floorf(pos.z / SPATIAL_CELL_SIZE);

    int n = 0;
    for (int ox = -1; ox <= 1; ox++)
        for (int oy = -1; oy <= 1; oy++)
            for (int oz = -1; oz <= 1; oz++)
                cell.neighborHashes[n++] = HashCoordsRaw(
                    cell.ix + ox, cell.iy + oy, cell.iz + oz);
    return cell;
}

u32 HashCoords(int x, int y, int z, u32 mask)
{
    unsigned int h = ((unsigned int)x * 73856093) ^
                     ((unsigned int)y * 19349663) ^
                     ((unsigned int)z * 83492791);
    return h & mask;
}

void SpatialTable_Init(SpatialTable *table, u32 buckets, u32 capacity)
{
    table->size = buckets;
    table->mask = buckets - 1;
    table->head = malloc(sizeof(u32) * buckets);
    table->value = malloc(sizeof(u32) * capacity);
    table->next = malloc(sizeof(u32) * capacity);
    table->capacity = capacity;
    SpatialTable_Clear(table);
}

void SpatialTable_Clear(SpatialTable *table)
{
    memset(table->head, 0xFF, sizeof(u32) * table->size);
    table->count = 0;
}

void SpatialTable_Free(SpatialTable *table)
{
    free(table->head);
    free(table->value);
    free(table->next);
}

void SpatialTable_Insert(SpatialTable *table, u32 hash, u32 value)
{
    if (table->count >= table->capacity)
        return;

    u32 idx = table->count++;
    table->value[idx] = value;
    table->next[idx] = table->head[hash & table->mask];
    table->head[hash & table->mask] = idx;
}
