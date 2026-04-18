```c
#include <math.h>
#include <stdbool.h>

// --- Placeholder Math Types (Assume these are defined elsewhere) ---
typedef struct { float x, y, z; } vec3;
typedef struct { float x, y, z; } quat;

// --- Components ---
typedef struct {
    vec3 pos;
    quat rot;
} Xform;

typedef struct {
    float height;
    float radius;
} Body;

typedef struct {
    vec3 vel;
    float mass;
} PhysicsState;

// --- Constants ---
#define RESTITUTION 0.8f // Bounciness
#define TIME_STEP 1.0f

// --- Utility Functions (Assume these are optimized) ---
static float vec3_distance_sq(vec3 a, vec3 b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return dx*dx + dy*dy + dz*dz;
}

static vec3 vec3_normalize(vec3 v) {
    float len = sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
    if (len == 0.0f) return (vec3){0, 0, 0};
    return (vec3){v.x/len, v.y/len, v.z/len};
}

// --- Core Physics System ---

/**
 * @brief Performs collision detection and response for two
bodies (A and B).
 *
 * Uses simplified sphere collision model for demonstration.
 * In a real system, this would handle capsule/box
intersection.
 *
 * @param xform_a, body_a, state_a Pointers to Body A's
components.
 * @param xform_b, body_b, state_b Pointers to Body B's
components.
 */
void resolve_collision(
    const Xform* xform_a, const Body* body_a, const PhysicsState* state_a,
    const Xform* xform_b, const Body* body_b, const PhysicsState* state_b)
{
    // 1. Collision Detection (Sphere approximation)
    vec3 delta = {
        xform_a->pos.x - xform_b->pos.x,
        xform_a->pos.y - xform_b->pos.y,
        xform_a->pos.z - xform_b->pos.z
    };

    float combined_radius = body_a->radius + body_b->radius;
    float combined_radius_sq = combined_radius *
combined_radius;
    float dist_sq = delta.x*delta.x + delta.y*delta.y +
delta.z*delta.z;

    if (dist_sq >= combined_radius_sq) {
        return; // No collision
    }

    // 2. Calculate Penetration and Normal
    float distance = sqrt(dist_sq);
    float penetration = combined_radius - distance;

    vec3 normal = {
        delta.x / distance,
        delta.y / distance,
        delta.z / distance
    };

    // 3. Position Correction (Separation)
    // Move objects apart proportional to their inverse mass
(or equally if masses are equal)
    float total_inv_mass = (1.0f / state_a->mass) + (1.0f / state_b->mass);
    float separation_a = (1.0f / state_a->mass) / total_inv_mass * penetration * 0.5f;
    float separation_b = (1.0f / state_b->mass) / total_inv_mass * penetration * 0.5f;

    // Apply separation (Requires mutable access to Xform positions)
    // NOTE: In a real ECS, this would write back to the component array.
    // For this function signature, we assume the caller handles the write-back.
    // xform_a->pos.x += normal.x * separation_a;
    // xform_a->pos.y += normal.y * separation_a;
    // xform_a->pos.z += normal.z * separation_a;

    // xform_b->pos.x -= normal.x * separation_b;
    // xform_b->pos.y -= normal.y * separation_b;
    // xform_b->pos.z -= normal.z * separation_b;


    // 4. Impulse Calculation (Velocity change)
    // Relative velocity along the normal
    float relative_velocity = (state_a.vel.x - state_b.vel.x) * normal.x +
                              (state_a.vel.y - state_b.vel.y) * normal.y +
                              (state_a.vel.z - state_b.vel.z) * normal.z;

    // Only apply impulse if objects are moving towards each
other
    if (relative_velocity > 0) return;

    // Coefficient of restitution (bounciness)
    float e = 0.8f;

    // Calculate impulse magnitude (J)
    float j = -(1.0f + e) * relative_velocity / (1.0f / state_a.mass + 1.0f / state_b.mass);

    // Apply impulse (Change in velocity)
    // state_a.vel += j * normal / state_a.mass;
    // state_b.vel -= j * normal / state_b.mass;
}


// --- Mock Structures for Compilation ---
// In a real system, these would be part of the main
component structure.
typedef struct { float x, y, z; } vec3;
typedef struct { vec3 vel; float mass; } State;

// --- Main Simulation Loop ---
void simulate_step(
    State* state_a, State* state_b,
    const Xform* xform_a, const Xform* xform_b)
{
    // 1. Apply forces/integration (Update velocity based on gravity, etc.)
    // (Skipped for brevity)

    // 2. Collision Detection & Resolution
    // (This function would iterate over all pairs of objects)
    // For demonstration, we call the resolution
function directly:
    resolve_collision(state_a, state_b, xform_a, xform_b);
}

// Mock Xform structure (Position/Orientation)
typedef struct { vec3 pos; } Xform;

// Mock function signature for collision resolution
void resolve_collision(State* state_a, State* state_b, const
Xform* xform_a, const Xform* xform_b) {
    // Placeholder for the complex collision logic
    // The actual implementation would use the math derived above.
    // resolve_collision(state_a, state_b, xform_a, xform_b);

}
```

```c
void Sol_System_Step_Physx_3d(World *world, double dt, double time)
{
    float fdt = (float)dt;
    int required = HAS_BODY3 | HAS_XFORM;
    WorldSpatial *ws = &world->worldSpatial;
    int count = world->activeCount;
    int i, j, k, l;
    int bodyCount = 0;

#pragma omp parallel for if (bodyCount > 500) schedule(guided)
    for (i = 0; i < count; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;

        CompBody *body = &world->bodies[id];
        CompXform *xform = &world->xforms[id];

        vec3s accel = SOL_PHYS_GRAV;
        accel = glms_vec3_add(accel, body->force);
        accel = glms_vec3_add(accel, body->impulse);
        body->impulse = (vec3s){0};

        body->vel = glms_vec3_add(body->vel, glms_vec3_scale(accel, fdt));
        body->grounded = 0;

        // Test at future position
        vec3s futurePos = glms_vec3_add(xform->pos, glms_vec3_scale(body->vel, fdt));
        SpatialCell cell = GetSpatialCell(futurePos);

        for (int n = 0; n < 27; n++)
        {
            u32 entry = ws->staticWorld.head[cell.neighborHashes[n]];
            while (entry != SPATIAL_NULL)
            {
                CollisionTri *tri = &ws->tris[ws->staticWorld.value[entry]];
                SolCollision col = ResolveSphereTriangle(body, &futurePos, tri);
                if (col.didCollide && col.normal.y > 0.5f)
                    body->grounded = 1;
                // Kill velocity into the surface
                float velInto = glms_vec3_dot(body->vel, col.normal);
                if (velInto < 0)
                    body->vel = glms_vec3_sub(body->vel,
                                              glms_vec3_scale(col.normal, velInto));

                entry = ws->staticWorld.next[entry];
            }
        }

        xform->pos = futurePos;

        if (body->invMass > 0.0f)
        {
            bodyIds[bodyCount] = id;
            cells[bodyCount] = GetSpatialCell(xform->pos);
            bodyCount++;
        }
    }

    // 3. Rebuild dynamic spatial + recache cells (positions changed from static resolve)
    Sol_Spatial_Build_Dynamic(world);
    for (int i = 0; i < bodyCount; i++)
        cells[i] = GetSpatialCell(world->xforms[bodyIds[i]].pos);

    // 4. Dynamic position resolution

#pragma omp parallel for if (bodyCount > 500) schedule(guided)
    for (k = 0; k < bodyCount; k++)
    {
        int id = bodyIds[k];
        CompBody *body = &world->bodies[id];
        CompXform *xform = &world->xforms[id];

        for (int n = 0; n < 27; n++)
        {
            u32 entry = ws->dynamicUnits.head[cells[k].neighborHashes[n]];
            while (entry != SPATIAL_NULL)
            {
                u32 otherID = ws->dynamicUnits.value[entry];
                if (id < (int)otherID)
                    resolve_position_only(body, xform,
                                        &world->bodies[otherID],
                                        &world->xforms[otherID]);
                entry = ws->dynamicUnits.next[entry];
            }
        }
    }

    // 5. Velocity resolution (reuse same cells — positions barely changed)
#pragma omp parallel for if (bodyCount > 500) schedule(guided)
    for (l = 0; l < bodyCount; l++)
    {
        int id = bodyIds[l];
        CompBody *body = &world->bodies[id];
        CompXform *xform = &world->xforms[id];

        for (int n = 0; n < 27; n++)
        {
            u32 entry = ws->dynamicUnits.head[cells[l].neighborHashes[n]];
            while (entry != SPATIAL_NULL)
            {
                u32 otherID = ws->dynamicUnits.value[entry];
                if (id < (int)otherID)
                    resolve_velocity_only(body, xform,
                                        &world->bodies[otherID],
                                        &world->xforms[otherID]);
                entry = ws->dynamicUnits.next[entry];
            }
        }
    }
}
```