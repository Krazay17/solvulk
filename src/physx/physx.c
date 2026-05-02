#include "sol_core.h"
#include <omp.h>

static u32         ents[MAX_ENTS];
static SolProfiler prof_static  = {.name = "Static"};
static SolProfiler prof_dynamic = {.name = "Dynamic"};

static int prof_frame = 0;

struct PhysxTable
{
    CompBody *bodies;
    u32       count;
};

void Physx_Init(World *world)
{
    world->spatial = calloc(1, sizeof(WorldPhysx));
    SpatialTable_Init(&world->spatial->staticGroup.table, SPATIAL_STATIC_SIZE, SPATIAL_STATIC_ENTRIES,
                      SPATIAL_STATIC_CELL_SIZE);
    SpatialTable_Init(&world->spatial->dynamicGroup.table, SPATIAL_DYNAMIC_SIZE, SPATIAL_DYNAMIC_ENTRIES,
                      SPATIAL_DYNAMIC_CELL_SIZE);
}

CompBody *Sol_Body_Add(World *world, int id, CompBody init_body)
{
    CompBody body     = init_body;
    body.height       = body.height ? body.height : 0.5f;
    body.radius       = body.radius ? body.radius : 0.5f;
    body.length       = body.length ? body.length : 0.5f;
    body.invMass      = body.mass > 0 ? 1.0f / body.mass : 0;
    world->bodies[id] = body;

    Spatial_Add(world, id, &world->bodies[id]);

    world->masks[id] |= HAS_BODY3;
    return &world->bodies[id];
}

void Physx_Step(World *world, double dt, double time)
{
    if (Sol_GetState()->fps < 30)
        return;
    float       fdt      = (float)dt;
    int         required = HAS_BODY3 | HAS_XFORM;
    int         i, j, k, l;
    int         count        = 0;
    int         activeCount  = world->activeCount;
    WorldPhysx *ws           = world->spatial;
    PhysxGroup *staticGroup  = &ws->staticGroup;
    PhysxGroup *dynamicGroup = &ws->dynamicGroup;

    Physx_Grid_Static_Rebuild(staticGroup);

    // Filter Entities
    for (i = 0; i < activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;
        if (world->bodies[id].mass == 0)
            continue;
        ents[count++] = id;
    }

    Ground_Trace(world, count);

    Prof_Begin(&prof_static);
    // velocity and static collisions
#pragma omp parallel for if (count > 500) schedule(dynamic, 16)
    for (j = 0; j < count; j++)
    {
        int        id    = ents[j];
        CompBody  *body  = &world->bodies[id];
        CompXform *xform = &world->xforms[id];

        if (xform->pos.y < -15)
        {
            Xform_Teleport(xform, (vec3s){0, 15, 0});
        }

        vec3s accel   = SOL_PHYS_GRAV;
        accel         = glms_vec3_add(accel, body->force);
        accel         = glms_vec3_add(accel, body->impulse);
        body->impulse = (vec3s){0};
        body->vel     = glms_vec3_add(body->vel, glms_vec3_scale(accel, fdt));

        SubstepData substep = Substep_Get(body, fdt);
        SolContact  hit     = {0};
        for (int s = 0; s < substep.substeps; s++)
        {
            xform->pos = glms_vec3_add(xform->pos, glms_vec3_scale(body->vel, substep.sub_dt));
            Collisions_Static_Grid(staticGroup, body, xform, shape_tri_test[body->shape], &hit);
        }
        if (hit.didCollide)
            Sol_Event_Add(world, (EventDesc){.pos = hit.point, .kind = EVENT_COLLISION});
    }
    Prof_EndEz(&prof_static, true);

    Fill_Dynamic_Table(world, count, ents);

    Prof_Begin(&prof_dynamic);
    // Dynamic collisions
#pragma omp parallel for if (count > 500) schedule(dynamic, 16)
    for (k = 0; k < count; k++)
    {
        int        id    = ents[k];
        CompBody  *body  = &world->bodies[id];
        CompXform *xform = &world->xforms[id];

        Collisions_Dynamic_Hashed(world, id, body, xform);
    }
    Prof_EndEz(&prof_dynamic, true);
}

void Ground_Trace(World *world, int count)
{
    u32 required = HAS_BODY3 | HAS_XFORM | HAS_MOVEMENT;
    int i;
#pragma omp parallel for if (count > 500) schedule(dynamic, 16)
    for (i = 0; i < count; i++)
    {
        int id = ents[i];
        if ((world->masks[id] & required) != required)
            continue;

        CompXform *xform = &world->xforms[id];
        CompBody  *body  = &world->bodies[id];

        // Start from center-bottom of the body
        vec3s origin = vecSub(xform->pos, vecSca(WORLD_UP, body->height * 0.25f));

        body->grounded = 0; // Reset every frame
        for (int j = 0; j < 4; j++)
        {
            // Rotate the local offset by the entity's rotation
            vec3s rotated_offset = glms_quat_rotatev(xform->quat, VECTOR_RADIAL_DIRECTIONS[j]);

            // Calculate ray start position
            vec3s pos = vecAdd(origin, vecSca(rotated_offset, body->radius));

            SolRayResult result = Sol_Raycast(
                world, (SolRay){.pos = pos, .dir = WORLD_DOWN, .dist = body->height * 0.3f, .ignoreEnt = id});

            if (result.hit && result.norm.y > 0.5f)
            {
                body->grounded     = 1;
                body->groundNormal = result.norm;
                break;
            }
        }
    }
}

SolRayResult Sol_Raycast(World *world, SolRay ray)
{
    SolRayResult result = {0};
    result.dist         = ray.dist;
    WorldPhysx *ws      = world->spatial;

    float dirLen = glms_vec3_norm(ray.dir);
    if (dirLen < FLOATING_EPSILON)
    {
        result.pos = ray.pos;
        return result;
    }
    ray.dir          = glms_vec3_scale(ray.dir, 1.0f / dirLen);
    result.pos       = glms_vec3_add(ray.pos, glms_vec3_scale(ray.dir, ray.dist));
    SolRayResult sub = {0};

    sub = Raycast_Static_Grid_Walk(world, ray);
    if (sub.hit && sub.dist < result.dist)
        result = sub;

    sub = Raycast_Dynamic_Table_Walk(world, ray);
    if (sub.hit && sub.dist < result.dist)
        result = sub;

    return result;
}

// dont fill pos or dir in ray
SolRayResult Sol_ScreenRaycast(World *world, float screenX, float screenY, SolRay ray)
{
    SolCamera *cam  = Sol_GetCamera();
    float      winW = Sol_GetState()->windowWidth;
    float      winH = Sol_GetState()->windowHeight;

    // 1. Convert screen pixel → NDC [-1, 1]
    float ndcX = (2.0f * screenX / winW) - 1.0f;
    float ndcY = (2.0f * screenY / winH) - 1.0f;
    // Note: your proj has the Y flip baked in (proj[1][1] *= -1),
    // so NDC Y here matches Vulkan's convention. No extra flip needed.

    // 2. Build a clip-space point at the far plane
    vec4 clipFar = {ndcX, ndcY, 1.0f, 1.0f};

    // 3. Invert viewProjection to go from clip → world
    mat4 invVP;
    glm_mat4_inv(cam->viewProj, invVP);

    // 4. Transform clip → world
    vec4 worldFar;
    glm_mat4_mulv(invVP, clipFar, worldFar);

    // 5. Perspective divide (homogeneous → 3D)
    if (fabsf(worldFar[3]) < FLOATING_EPSILON)
    {
        return (SolRayResult){0};
    }
    vec3s farPos = {{
        worldFar[0] / worldFar[3],
        worldFar[1] / worldFar[3],
        worldFar[2] / worldFar[3],
    }};

    // 6. Build the ray from camera position toward the far point
    vec3s camPos = {{cam->position[0], cam->position[1], cam->position[2]}};
    vec3s dir    = glms_vec3_normalize(glms_vec3_sub(farPos, camPos));

    ray.pos = camPos;
    ray.dir = dir;

    // 7. Cast through your existing raycast
    return Sol_Raycast(world, ray);
}

CompBody *Sol_Body2_Add(World *world, int id)
{
    world->masks[id] |= HAS_BODY2;
    return &world->bodies[id];
}

void Sol_System_Step_Physx_2d(World *world, double dt, double time)
{
    float fdt      = (float)dt;
    int   required = HAS_BODY2 | HAS_XFORM;
    for (int i = 0; i < world->activeCount; ++i)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) == required)
        {
            CompBody     *body     = &world->bodies[id];
            CompMovement *movement = &world->movements[id];
            CompXform    *xform    = &world->xforms[id];

            vec3s accel   = SOL_PHYS_GRAV;
            accel         = glms_vec3_add(accel, body->force);
            accel         = glms_vec3_add(accel, body->impulse);
            body->impulse = (vec3s){0};
            body->vel     = glms_vec3_add(body->vel, glms_vec3_scale(accel, fdt));

            if (xform->pos.y + body->height >= Sol_GetState()->windowHeight)
            {
                xform->pos.y = Sol_GetState()->windowHeight - body->height;
                body->vel.y  = 0;
            }
        }
    }
}