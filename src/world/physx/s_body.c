#include "physx_i.h"
#include "sol_engine.h"
#include "sol_math.h"
#include "sol_core.h"
#include "world.h"
#include "profiler.h"
#include "xform/s_xform.h"
#include "event/s_event.h"
#include "owner/s_owner.h"
#include "replication/s_replication.h"
#include <omp.h>

#define SPATIAL_DYNAMIC_CELL_SIZE 1.0f
#define SPATIAL_DYNAMIC_SIZE (1 << 18)
#define SPATIAL_DYNAMIC_ENTRIES 0x2FFFF

// Cell 1.5, Size (1<<21) loads slow but plays fast
#define SPATIAL_STATIC_SIZE (1 << 19)
#define SPATIAL_STATIC_ENTRIES 0xF

static u32            ents[MAX_ENTS];
static EntityContacts contacts[MAX_ENTS];

static SolProfiler prof_static  = {.name = "Static"};
static SolProfiler prof_dynamic = {.name = "Dynamic"};

static int prof_frame = 0;

void Sol_Physx_Init(World *world)
{
    world->bodies  = calloc(MAX_ENTS, sizeof(CompBody));
    world->spatial = calloc(1, sizeof(WorldPhysx));

    // SpatialTable_Init(&world->spatial->staticGroup.table, SPATIAL_STATIC_SIZE, SPATIAL_STATIC_ENTRIES,
    //                   SPATIAL_STATIC_CELL_SIZE);
    SpatialTable_Init(&world->spatial->dynamicGroup.table, SPATIAL_DYNAMIC_SIZE, SPATIAL_DYNAMIC_ENTRIES,
                      SPATIAL_DYNAMIC_CELL_SIZE);

    WAddStep(world) = Sol_Physx_Step;
}

void Sol_Body_Add(World *world, int id, BodyDesc desc)
{
    world->masks[id] |= BITC(HAS_BODY3);

    CompBody body = {
        .mass           = desc.mass,
        .shape          = desc.shape,
        .group          = desc.group,
        .base_group     = desc.group,
        .gravity        = SOL_PHYS_GRAV,
        .invMass        = desc.mass > 0 ? 1.0f / desc.mass : 0,
        .restitution    = desc.restitution ? desc.restitution : 0.5f,
        .vel            = desc.vel,
        .ignoreFriendly = desc.ignoreFriendly,
        .dims =
            (vec3s){
                .x = desc.radius,
                .y = desc.height,
                .z = desc.length,
            },
    };
    world->bodies[id] = body;
    Spatial_Add(world, id, &world->bodies[id]);
}

void Sol_Physx_Step(World *world, double dt, double time)
{
    if (solState.fps < 30)
        return;
    float       fdt      = (float)dt;
    int         required = BITC(HAS_BODY3);
    int         i, j, k, l;
    int         count        = 0;
    int         activeCount  = world->activeCount;
    WorldPhysx *ws           = world->spatial;
    PhysxGroup *staticGroup  = &ws->staticGroup;
    PhysxGroup *dynamicGroup = &ws->dynamicGroup;

    memset(contacts, 0, sizeof(contacts));

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

    Physx_Grid_Static_Rebuild(staticGroup);
    Fill_Dynamic_Table(world, count, ents);

    // Prof_Begin(&prof_static);
    //  velocity and static collisions
#pragma omp parallel for if (count > 200) shared(contacts) schedule(dynamic, 16)
    for (j = 0; j < count; j++)
    {
        int        id    = ents[j];
        CompBody  *body  = &world->bodies[id];
        CompXform *xform = &world->xforms[id];

        if (world->replications[id].auth == NETAUTH_REMOTE)
        {
            xform->pos = glms_vec3_add(xform->pos, glms_vec3_scale(body->vel, fdt));
            continue;
        }

        if (xform->pos.y < -250.0f)
        {
            if (world->flags[id].flags & EFLAG_PROJECTILE)
            {
                Sol_Destroy_Ent(world, id);
                continue;
            }
            Sol_Xform_Teleport(world, id, (vec3s){0, 15, 0});
        }
        if (body->vel.y < -100.0f)
            body->vel.y = -100.0f;

        vec3s accel   = body->gravity;
        accel         = glms_vec3_add(accel, body->force);
        accel         = glms_vec3_add(accel, body->impulse);
        body->impulse = (vec3s){0};
        body->vel     = glms_vec3_add(body->vel, glms_vec3_scale(accel, fdt));

        SubstepData substep = Substep_Get(body, fdt);
        for (int s = 0; s < substep.substeps; s++)
        {
            xform->pos = glms_vec3_add(xform->pos, glms_vec3_scale(body->vel, substep.sub_dt));
            Collisions_Static_Grid(world, staticGroup, body, xform, &contacts[j]);
        }

        // Spatial_Table_Dynamic_Single(&dynamicGroup->table, id, xform->pos, body->dims.x, body->dims.y);
        Collisions_Dynamic_Hashed(world, id, body, xform, &contacts[j]);
    }
    // Prof_EndEz(&prof_static, true);

    for (int i = 0; i < count; i++)
    {
        int id = ents[i];
        for (int j = 0; j < contacts[i].count; j++)
        {
            SolContact *r = &contacts[i].records[j];
            Sol_Event_Add(world, (SolEvent){
                                     .kind = EVENTKIND_COLLISION,
                                     .as.collision =
                                         {
                                             .entA   = id,
                                             .entB   = r->entId,
                                             .normal = r->normal,
                                             .pos    = r->pos,
                                         },
                                 });
        }
    }
}

float Sol_Physx_Get_Ground_Dot(World *world, int id)
{
    CompXform *xform = &world->xforms[id];
    CompBody  *body  = &world->bodies[id];

    // Start from center-bottom of the body
    vec3s origin       = vecAdd(xform->pos, vecSca(WORLD_DOWN, body->dims.y * 0.4f));
    body->groundNormal = (vec3s){0, 0, 0};
    SolRayResult results[9];
    for (int j = 0; j < 9; j++)
    {
        // Rotate the local offset by the entity's rotation
        vec3s rotated_offset = glms_quat_rotatev(xform->quat, VECTOR_RADIAL_DIRECTIONS[j]);

        vec3s pos = vecAdd(origin, vecSca(rotated_offset, body->dims.x * 0.95f));

        results[j] = Sol_Raycast(
            world, (SolRay){.pos = pos, .dir = WORLD_DOWN, .dist = body->dims.y * 0.3f, .ignoreEnt = id, .mask = 0});
    }
    float flattestNorm = -1.0f;
    int   idx          = 0;
    for (int j = 0; j < 9; j++)
    {
        if (results[j].norm.y > flattestNorm)
        {
            flattestNorm = results[j].norm.y;
            idx          = j;
        }
    }
    body->groundNormal = results[idx].norm;
    return flattestNorm;
}

SolRayResult Sol_Raycast(World *world, SolRay ray)
{
    WorldPhysx *ws = world->spatial;

    SolRayResult result = {
        .dist = ray.dist,
        .norm = {0.0f, 0.0f, 0.0f},
        .pos  = glms_vec3_add(ray.pos, glms_vec3_scale(ray.dir, ray.dist)),
    };

    SolRayResult sub = {0};

    sub = Raycast_Static_Grid_Walk(world, ray);
    if (sub.hit && sub.dist < result.dist)
        result = sub;

    sub = Raycast_Dynamic_Table_Walk(world, ray);
    if (sub.hit && sub.dist < result.dist)
        result = sub;

    return result;
}

int Sol_SphereCast(World *world, SolRay ray, float radius, SolRayResult *results, int maxResults)
{
    if (!results || maxResults <= 0)
        return 0;

    int hitCount = 0;

    // Normalize the ray direction
    vec3s rayDir   = glms_vec3_normalize(ray.dir);
    float rayLen   = ray.dist;
    vec3s sweepEnd = glms_vec3_add(ray.pos, glms_vec3_scale(rayDir, rayLen));

    // Broadphase: build AABB around the entire sweep + radius margin
    vec3s expand = (vec3s){{radius, radius, radius}};
    vec3s boxMin = glms_vec3_sub(glms_vec3_minv(ray.pos, sweepEnd), expand);
    vec3s boxMax = glms_vec3_add(glms_vec3_maxv(ray.pos, sweepEnd), expand);

    int candidates[256];
    int candCount = Spatial_QueryEntsAABB(&world->spatial->dynamicGroup.table, boxMin, boxMax, candidates, 256);

    for (int i = 0; i < candCount; i++)
    {
        int otherID = candidates[i];

        if (!(world->masks[otherID] & BITC(HAS_BODY3)))
            continue;
        if (otherID == ray.ignoreEnt)
            continue;

        CompBody  *body  = &world->bodies[otherID];
        CompXform *xform = &world->xforms[otherID];

        SolRayResult hit;
        bool         didHit = false;

        switch (body->shape)
        {
        case SHAPE3_SPH:
            didHit = SphereCast_VsSphere(ray.pos, rayDir, rayLen, radius, xform->pos, body->dims.x, &hit);
            break;
        case SHAPE3_CAP:
            didHit =
                SphereCast_VsCapsule(ray.pos, rayDir, rayLen, radius, xform->pos, body->dims.x, body->dims.y, &hit);
            break;
        }

        if (didHit && hitCount < maxResults)
        {
            hit.entId           = otherID;
            results[hitCount++] = hit;
        }
    }

    return hitCount;
}

// dont fill pos or dir in ray
SolRayResult Sol_ScreenRaycast(World *world, int screenX, int screenY, SolRay ray)
{
    float winW = (float)solEngine.windowWidth;
    float winH = (float)solEngine.windowHeight;

    // 1. Convert screen pixel → NDC [-1, 1]
    float ndcX = (2.0f * screenX / winW) - 1.0f;
    float ndcY = (2.0f * screenY / winH) - 1.0f;
    // Note: your proj has the Y flip baked in (proj[1][1] *= -1),
    // so NDC Y here matches Vulkan's convention. No extra flip needed.

    // 2. Build a clip-space point at the far plane
    vec4s clipFar = {ndcX, ndcY, 1.0f, 1.0f};

    // 3. Invert viewProjection to go from clip → world
    mat4s invVP;
    invVP = glms_mat4_inv(Sol_Cam_GetViewProj());

    // 4. Transform clip → world
    vec4s worldFar;
    worldFar = glms_mat4_mulv(invVP, clipFar);

    // 5. Perspective divide (homogeneous → 3D)
    if (fabsf(worldFar.raw[3]) < FLOATING_EPSILON)
    {
        return (SolRayResult){0};
    }
    vec3s farPos = {{
        worldFar.x / worldFar.w,
        worldFar.y / worldFar.w,
        worldFar.z / worldFar.w,
    }};

    // 6. Build the ray from camera position toward the far point
    vec3s camPos = Sol_Cam_GetPos();
    vec3s dir    = glms_vec3_normalize(glms_vec3_sub(farPos, camPos));

    ray.pos = camPos;
    ray.dir = dir;

    // 7. Cast through your existing raycast
    return Sol_Raycast(world, ray);
}

vec3s Sol_Physx_GetVel(World *world, int id)
{
    return world->bodies[id].vel;
}

vec3s Sol_Physx_GetVelDir(World *world, int id)
{
    return glms_vec3_normalize(world->bodies[id].vel);
}

float Sol_Physx_GetSpeed(World *world, int id)
{
    return glms_vec3_norm(Sol_Physx_GetVel(world, id));
}

float Sol_Physx_GetLatSpeed(World *world, int id)
{
    vec3s vel = Sol_Physx_GetVel(world, id);
    vel.y     = 0;
    return glms_vec3_norm(vel);
}

void Sol_Physx_SetGrav(World *world, int id, vec3s vel)
{
    world->bodies[id].gravity = vel;
}

vec3s Sol_Physx_GetDims(World *world, int id)
{
    return world->bodies[id].dims;
}

vec3s Sol_Physx_GetGround(World *world, int id)
{
    return world->bodies[id].groundNormal;
}
void Sol_Physx_AddVel(World *world, int id, vec3s addvel)
{
    vec3s *vel = &world->bodies[id].vel;
    *vel       = vecAdd(*vel, addvel);
}
void Sol_Physx_SetVel(World *world, int id, vec3s vel)
{
    world->bodies[id].vel = vel;
}
void Sol_Physx_BlendVel(World *world, int id, vec3s vel, float amnt)
{
    glms_vec3_lerp(world->bodies[id].vel, vel, amnt);
}
void Sol_Physx_SetVellat(World *world, int id, vec3s vel)
{
    world->bodies[id].vel.x = vel.x;
    world->bodies[id].vel.z = vel.z;
}
void Sol_Physx_SetVelX(World *world, int id, float x)
{
    world->bodies[id].vel.x = x;
}
void Sol_Physx_SetVelY(World *world, int id, float y)
{
    world->bodies[id].vel.y = y;
}
void Sol_Physx_SetVelZ(World *world, int id, float z)
{
    world->bodies[id].vel.z = z;
}

vec3s Sol_Physx_GetHeadPos(World *world, int id)
{
    vec3s head = world->xforms[id].pos;
    head.y += world->bodies[id].dims.y * 0.4f;
    return head;
}
void Sol_Physx_Impulse(World *world, int id, vec3s impulse)
{
    world->bodies[id].impulse = impulse;
}
float Sol_Physx_GetHeight(World *world, int id)
{
    return world->bodies[id].dims.y;
}
void Sol_Physx_SetHeight(World *world, int id, float height)
{
    world->bodies[id].dims.y = height;
}
void Sol_Physx_SetRedirectVel(World *world, int id, vec3s dir)
{
    vec3s prevVel         = world->bodies[id].vel;
    float mag             = glms_vec3_norm(prevVel);
    world->bodies[id].vel = vecSca(glms_vec3_normalize(dir), mag);
}
void Sol_Physx_LerpVel(World *world, int id, vec3s vel, float amnt)
{
    world->bodies[id].vel = glms_vec3_lerp(world->bodies[id].vel, vel, amnt);
}
bool Sol_Physx_DoesCollide(World *world, int id, int idB)
{
    if (!Sol_Owner_GetHostile(world, id, idB) &&
        (world->bodies[id].ignoreFriendly || world->bodies[idB].ignoreFriendly))
        return false;
    return world->bodies[id].group << 16 & world->bodies[idB].group;
}
bool Sol_Physx_DoesRayCollide(World *world, int id, int idB)
{
    if (!Sol_Owner_GetHostile(world, id, idB) &&
        (world->bodies[id].ignoreFriendly || world->bodies[idB].ignoreFriendly))
        return false;
    return world->bodies[id].ray_group << 16 & world->bodies[idB].ray_group;
}