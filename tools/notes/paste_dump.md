

### CODE DUMP
```c
float dir = target > button->hoverAnim[i] ? 1.0f : -1.0f;
button->hoverAnim[i] = fminf(fmaxf(button->hoverAnim[i] + dir * dt * speed, 0.0f), 1.0f);


WorldCollider worldCollider = {0};

void Sol_Physics_BuildWorldCollider(World *world, SolModel *model)
{
WorldCollider *WorldCollider = &world->worldSpatial.staticWorld;

// Count triangles
uint32_t totalTris = 0;
for (uint32_t m = 0; m < model->meshCount; m++)
    totalTris += model->meshes[m].indexCount / 3;

worldCollider.tris = malloc(sizeof(CollisionTri) * totalTris);
worldCollider.triCount = totalTris;
memset(worldCollider.cells, 0, sizeof(worldCollider.cells));

// Extract triangles
uint32_t triIdx = 0;
for (uint32_t m = 0; m < model->meshCount; m++)
{
    SolMesh *mesh = &model->meshes[m];
    for (uint32_t i = 0; i < mesh->indexCount; i += 3)
    {
        SolVertex *va = &model->vertices[mesh->vertexOffset + model->indices[mesh->indexOffset + i + 0]];
        SolVertex *vb = &model->vertices[mesh->vertexOffset + model->indices[mesh->indexOffset + i + 1]];
        SolVertex *vc = &model->vertices[mesh->vertexOffset + model->indices[mesh->indexOffset + i + 2]];

        CollisionTri *tri = &worldCollider.tris[triIdx];
        tri->a = (vec3s){va->position[0], va->position[1], va->position[2]};
        tri->b = (vec3s){vb->position[0], vb->position[1], vb->position[2]};
        tri->c = (vec3s){vc->position[0], vc->position[1], vc->position[2]};

        // Face normal
        vec3s edge1 = glms_vec3_sub(tri->b, tri->a);
        vec3s edge2 = glms_vec3_sub(tri->c, tri->a);
        tri->normal = glms_vec3_normalize(glms_vec3_cross(edge1, edge2));

        // Insert into every cell the triangle's AABB touches
        float minX = fminf(fminf(tri->a.x, tri->b.x), tri->c.x);
        float minY = fminf(fminf(tri->a.y, tri->b.y), tri->c.y);
        float minZ = fminf(fminf(tri->a.z, tri->b.z), tri->c.z);
        float maxX = fmaxf(fmaxf(tri->a.x, tri->b.x), tri->c.x);
        float maxY = fmaxf(fmaxf(tri->a.y, tri->b.y), tri->c.y);
        float maxZ = fmaxf(fmaxf(tri->a.z, tri->b.z), tri->c.z);

        int x0 = (int)floorf(minX / WORLD_CELL_SIZE);
        int y0 = (int)floorf(minY / WORLD_CELL_SIZE);
        int z0 = (int)floorf(minZ / WORLD_CELL_SIZE);
        int x1 = (int)floorf(maxX / WORLD_CELL_SIZE);
        int y1 = (int)floorf(maxY / WORLD_CELL_SIZE);
        int z1 = (int)floorf(maxZ / WORLD_CELL_SIZE);

        for (int x = x0; x <= x1; x++)
            for (int y = y0; y <= y1; y++)
                for (int z = z0; z <= z1; z++)
                {
                    WorldCell *cell = &worldCollider.cells[WorldHash(x, y, z)];
                    if (cell->count < MAX_TRIS_PER_CELL)
                        cell->triIndices[cell->count++] = triIdx;
                }

        triIdx++;
    }
}
printf("WorldCollider: %u triangles\n", totalTris);
}


void ResolveWorldCollisions(World *world)
{
int required = HAS_BODY3 | HAS_XFORM;

for (int i = 0; i < world->activeCount; i++)
{
    int id = world->activeEntities[i];
    if ((world->masks[id] & required) != required || world->bodies[id].invMass <= 0.0f)
        continue;

    CompBody *body = &world->bodies[id];
    CompXform *xform = &world->xforms[id];

    float r = body->radius;
    vec3s pos = xform->pos;
    bool foundGround = false;

    // Check cells the sphere could touch
    int x0 = (int)floorf((pos.x - r) / WORLD_CELL_SIZE);
    int y0 = (int)floorf((pos.y - r) / WORLD_CELL_SIZE);
    int z0 = (int)floorf((pos.z - r) / WORLD_CELL_SIZE);
    int x1 = (int)floorf((pos.x + r) / WORLD_CELL_SIZE);
    int y1 = (int)floorf((pos.y + r) / WORLD_CELL_SIZE);
    int z1 = (int)floorf((pos.z + r) / WORLD_CELL_SIZE);

    for (int x = x0; x <= x1; x++)
        for (int y = y0; y <= y1; y++)
            for (int z = z0; z <= z1; z++)
            {
                WorldCell *cell = &worldCollider.cells[WorldHash(x, y, z)];
                for (uint32_t n = 0; n < cell->count; n++)
                {
                    CollisionTri *tri = &worldCollider.tris[cell->triIndices[n]];
                    vec3s closest = ClosestPointOnTriangle(pos, tri->a, tri->b, tri->c);
                    vec3s delta = glms_vec3_sub(pos, closest);
                    float distSq = glms_vec3_dot(delta, delta);

                    if (distSq >= (r + SOL_PHYS_COLLISION_SKIN) * (r + SOL_PHYS_COLLISION_SKIN) ||
                        distSq <= 0.0f)
                        continue;

                    float dist = sqrtf(distSq);
                    vec3s normal = glms_vec3_scale(delta, 1.0f / dist);
                    float penetration = r - dist;

                    // Push out
                    xform->pos = glms_vec3_add(xform->pos, glms_vec3_scale(normal, penetration));

                    // Remove velocity into the surface
                    float velIntoSurface = glms_vec3_dot(body->vel, normal);
                    if (velIntoSurface < 0.0f)
                        body->vel = glms_vec3_sub(body->vel, glms_vec3_scale(normal, velIntoSurface));

                    // Grounding check (surface pointing mostly up)
                    if (normal.y > 0.5f)
                    {
                        foundGround = true;
                    }
                }
            }
    if (foundGround)
    {
        body->grounded += 0.016f;
        body->airtime = 0;
    }
    else
    {
        body->airtime += 0.016f;
        body->grounded = 0;
    }
}
}


int GetHash(vec3s pos)
{
    // Quantize position into integer grid coordinates
    int ix = (int)floorf(pos.x / CELL_SIZE);
    int iy = (int)floorf(pos.y / CELL_SIZE);
    int iz = (int)floorf(pos.z / CELL_SIZE);

    // Spatial Hash formula (using primes)
    unsigned int hash = ((unsigned int)ix * 73856093) ^
                        ((unsigned int)iy * 19349663) ^
                        ((unsigned int)iz * 83492791);

    return hash % TABLE_SIZE;
}

void BuildSpatialHash(World *world, SpatialHash *grid)
{
    memset(grid->cells, 0, sizeof(SpatialCell) * TABLE_SIZE);

    for (int i = 0; i < world->activeCount; ++i)
    {
        int id = world->activeEntities[i];
        if (!(world->masks[id] & HAS_BODY3))
            continue;

        int hash = GetHash(world->xforms[id].pos);
        SpatialCell *cell = &grid->cells[hash];

        // atomic_fetch_add here
        if (cell->count < MAX_ENTITIES_PER_CELL)
        {
            cell->entityIds[cell->count++] = id;
        }
    }
}

void ResolveCollisionsSpatial(World *world, SpatialHash *grid)
{
    int i;
    int count = world->activeCount;
#pragma omp parallel for if (count > 1000) schedule(guided)
    for (i = 0; i < count; i++)
    {
        int idA = world->activeEntities[i];
        if (!(world->masks[idA] & HAS_BODY3))
            continue;

        vec3s posA = world->xforms[idA].pos;
        int ix = (int)floorf(posA.x / CELL_SIZE);
        int iy = (int)floorf(posA.y / CELL_SIZE);
        int iz = (int)floorf(posA.z / CELL_SIZE);

        for (int ox = -1; ox <= 1; ox++)
            for (int oy = -1; oy <= 1; oy++)
                for (int oz = -1; oz <= 1; oz++)
                {
                    unsigned int hash = ((unsigned int)(ix + ox) * 73856093) ^
                                        ((unsigned int)(iy + oy) * 19349663) ^
                                        ((unsigned int)(iz + oz) * 83492791);

                    SpatialCell *cell = &grid->cells[hash % TABLE_SIZE];

                    // PRE-FETCH count to avoid cache misses in the loop
                    int cellCount = cell->count;
                    for (int n = 0; n < cellCount; n++)
                    {
                        int idB = cell->entityIds[n];

                        // This is the most important line for performance:
                        // It ensures we only check the pair once AND avoids self-collision.
                        if (idA >= idB)
                            continue;

                        ResolveCollision(&world->bodies[idA], &world->xforms[idA],
                                         &world->bodies[idB], &world->xforms[idB]);
                    }
                }
    }
}


void ResolveSphereMesh(CompBody *sphereBody, CompXform *sphereXform, CompModel *meshModel, CompXform *meshXform)
{
    // 1. Move sphere center into Mesh Local Space
    // This allows us to use raw vertex data without transforming every triangle
    vec3s localSpherePos = glms_vec3_sub(sphereXform->pos, meshXform->pos);

    // (Optional: If your mesh is rotated, you must rotate localSpherePos by the inverse quat here)

    for (int i = 0; i < meshModel->model->totalVertices; i += 3)
    {
        // 2. Correctly extract raw Local Vertices from the mesh
        // We cast the float array directly to vec3s for the math function
        vec3s a = *(vec3s *)meshModel->model->vertices[i + 0].position;
        vec3s b = *(vec3s *)meshModel->model->vertices[i + 1].position;
        vec3s c = *(vec3s *)meshModel->model->vertices[i + 2].position;

        // 3. Narrowphase math
        vec3s closestP = ClosestPointOnTriangle(localSpherePos, a, b, c);
        vec3s delta = glms_vec3_sub(localSpherePos, closestP);
        float distSq = glms_vec3_dot(delta, delta);
        float radius = sphereBody->radius;

        if (distSq < radius * radius)
        {
            float dist = sqrtf(distSq);
            vec3s normal;
            if (dist > 0.0001f)
            {
                normal = glms_vec3_scale(delta, 1.0f / dist);
            }
            else
            {
                // If the sphere is exactly on the face, the normal is the triangle normal
                vec3s edge1 = glms_vec3_sub(b, a);
                vec3s edge2 = glms_vec3_sub(c, a);
                normal = glms_vec3_normalize(glms_vec3_cross(edge1, edge2));
            }
            float penetration = radius - dist;
            // 4. Correct position in Local Space
            localSpherePos = glms_vec3_add(localSpherePos, glms_vec3_scale(normal, penetration));

            // 5. Velocity Reflection
            // Note: Since the floor doesn't move, we only reflect the sphere's velocity
            float velAlongNormal = glms_vec3_dot(sphereBody->vel, normal);
            if (velAlongNormal < 0)
            {
                float j = -(1.0f + sphereBody->restitution) * velAlongNormal;
                sphereBody->vel = glms_vec3_add(sphereBody->vel, glms_vec3_scale(normal, j));
            }
        }
    }

    // 6. Move the corrected position back to World Space
    sphereXform->pos = glms_vec3_add(localSpherePos, meshXform->pos);
}


void Spatial_Insert(SpatialTable *table, vec3s pos, CompBody *body, u32 value)
{
    if (table->count >= SPATIAL_DYNAMIC_ENTRIES)
        return; // Table full!

    int ix = (int)floorf(pos.x / SPATIAL_CELL_SIZE);
    int iy = (int)floorf(pos.y / SPATIAL_CELL_SIZE);
    int iz = (int)floorf(pos.z / SPATIAL_CELL_SIZE);
    u32 hash = HashCoords(ix, iy, iz);
    int n = 0;
    for (int ox = -1; ox <= 1; ox++)
        for (int oy = -1; oy <= 1; oy++)
            for (int oz = -1; oz <= 1; oz++)
                body->neighborHashes[n++] = HashCoords(ix + ox, iy + oy, iz + oz);

    Get a new entry from the pool
    u32 entryIdx = table->count++;
    table->value[entryIdx] = value;

    Link it: New entry points to the old head, head points to new entry
    table->next[entryIdx] = table->head[hash];
    table->head[hash] = entryIdx;
}


#include "sol.h"

#include "ui/sol_ui.h"

#define MODEL_COUNT 100

static enum GameButtons {
    GAME_BUTTON_QUIT,
    GAME_BUTTON_TEST1,
    GAME_BUTTON_TEST2,
    GAME_BUTTON_TEST3,
    GAME_BUTTON_TEST4,
    GAME_BUTTON_TEST5,
    GAME_BUTTON_TEST6,
    GAME_BUTTON_TEST7,
    GAME_BUTTON_TEST8,
    GAME_BUTTON_TEST9,
    GAME_BUTTON_COUNT,
} GameButtons;

static vec3 playerPos = {0, 4, 10};
static float rotation = 0.0f;
static float camPos = 10.0f;
static SolButton buttons[GAME_BUTTON_COUNT] = {0};

static void Init();
static void Tick(double dt, double time);
static void Draw();
static void Size(float width, float height);

World game = {
    .init = Init,
    .tick = Tick,
    .draw = Draw,
    .size = Size,
};

World *Sol_GetGame(void)
{
    return &game;
}

static void Init()
{
    game.active = true;

    for (int i = 0; i < GAME_BUTTON_COUNT; ++i)
    {
        int cols = 5;
        int col = i % cols;
        int row = i / cols;
        float startX = 100.0f;
        float startY = 200.0f;
        float xOffset = col * 110 + startX;
        float yOffset = row * 60 + startY;
        buttons[i].rect = (SolRect){xOffset, yOffset, 100, 50};
        buttons[i].color = (SolColor){255, 0, 0, 55};
        buttons[i].drawOrder = i;

        switch (i)
        {
        case GAME_BUTTON_QUIT:
            Sol_Button_InitText(&buttons[i], (SolColor){0, 255, 255, 255}, "QUIT", 16.0f);
            buttons[i].color = (SolColor){255, 0, 0, 255};
            buttons[i].callback = Sol_Shutdown;
            break;
        default:
            Sol_Button_InitText(&buttons[i], (SolColor){0, 255, 255, 255}, "BUTTON", 16.0f);
        }
    }
}

static void Tick(double dt, double time)
{
    rotation += 5.0f * dt;

    PlayerState *playerState = Sol_GetPlayerState();
    if (playerState->actionState & ACTION_FWD)
    {
        playerPos[2] -= 10.0f * dt;
    }
    else if (playerState->actionState & ACTION_BWD)
    {
        playerPos[2] += 10.0f * dt;
    }

    Sol_Camera_Update((vec3){0, 7, playerPos[2]}, (vec3){0, 0, 0});

    Sol_Button_Update(buttons, 0, GAME_BUTTON_COUNT, dt);
}

static void Draw()
{
    for (int i = 0; i < MODEL_COUNT; ++i)
        Sol_DrawModel(Sol_Loader_GetBank()->models.wizard, (vec3){sin(i), -1, -i}, rotation);
    Sol_DrawModel(Sol_Loader_GetBank()->models.world0, (vec3){0,0,0}, 0);

    Sol_Button_Draw(buttons, 0, GAME_BUTTON_COUNT);
}

static void Size(float width, float height)
{
    
}

// flip bool on pointer
World *menu = Sol_GetMenu();
menu->active = !menu->active;
worlds[0]->active ^= 1;

// SolCreateBuffer gives you the memory. 
// Descriptors tell the shader where that memory is. 
// That's what Sol_CreateDescriptorBuffer does — it calls SolCreateBuffer to get the memory, 
// then creates a descriptor that points the shader to it.

#pragma omp parallel for if (count > 100) default(none) \
    shared(world, ws, count, required, fdt)             \
    private(i)                                          \
    schedule(guided)

// Inside Sol_System_Step_Physx_3d
SpatialCell cell = GetSpatialCell(xform->pos);
u32 potentialTris[256]; // Max tris to check per frame
int triCount = 0;

// Gather unique triangles once per entity
for (int n = 0; n < 27 && triCount < 256; n++) {
    u32 entry = ws->staticWorld.head[cell.neighborHashes[n]];
    while (entry != SPATIAL_NULL && triCount < 256) {
        potentialTris[triCount++] = ws->staticWorld.value[entry];
        entry = ws->staticWorld.next[entry];
    }
}

// Now substep only the gathered list
for (int s = 0; s < substeps; s++) {
    xform->pos = glms_vec3_add(xform->pos, glms_vec3_scale(body->vel, subDt));
    for (int t = 0; t < triCount; t++) {
        CollisionTri *tri = &ws->tris[potentialTris[t]];
        
        // Quick AABB check here would also help!
        SolCollision col = ResolveSphereTriangle(body, &xform->pos, tri);
        if (col.didCollide && col.normal.y > 0.5f)
            body->grounded = 1;
    }
}

```