#include "world.h"
#include "sol_math.h"
#include "render/render.h"

const float jitter_mag = 0.33f;

Ribbon ribbon_kinds[RIBBONKIND_COUNT] = {
    [RIBBONKIND_BASIC] =
        {
            .color       = {1.0f, 1.0f, 1.0f, 1.0f},
            .lifespan    = 5.0f,
            .scale_curve = CURVE_EASE_OUT,
            .alpha_curve = CURVE_EASE_OUT,
            .texture     = SOL_TEXTURE_BEAM,
            .thickness   = 1.0f,
            .pan         = 1.0f,
        },
    [RIBBONKIND_LIGHTNING] =
        {
            .color       = {1.0f, 1.0f, 1.0f, 1.0f},
            .lifespan    = 0.2f,
            .scale_curve = CURVE_EASE_OUT,
            .alpha_curve = CURVE_EASE_OUT,
            .texture     = SOL_TEXTURE_LIGHTNING,
            .thickness   = 1.0f,
            .pan         = 2.0f,
        },
};

void SlRibbon_Init(World *world, SlRibbon *self)
{
    solb_init(self->ribbons, 64);
}
void SlRibbon_Deinit(SlRibbon *self)
{
    solb_free(self->ribbons);
}

// Helper to generate jagged points between A and B
static void GenerateLightningPoints(Ribbon *r)
{
    vec3s start = r->points[0];
    vec3s end   = r->points[r->_point_count - 1]; // Keep the current target

    r->_point_count                    = MAX_RIBBON_SEGMENTS;
    r->points[0]                       = start;
    r->points[MAX_RIBBON_SEGMENTS - 1] = end;

    vec3s dir = vecSub(end, start);
    // Simple perpendicular vectors for jitter (assuming Y is up)
    vec3s right = vecNorm(vecCross(dir, WORLD_UP));
    if (vecLen(right) < 0.01f)
        right = (vec3s){1.0f, 0, 0}; // Fallback if pointing straight up
    vec3s up = vecNorm(vecCross(right, dir));

    for (int i = 1; i < MAX_RIBBON_SEGMENTS - 1; i++)
    {
        float t        = (float)i / (float)(MAX_RIBBON_SEGMENTS - 1);
        vec3s base_pos = vecAdd(start, vecSca(dir, t));

        // Random offset between -0.5 and 0.5, scaled by distance
        float rand_r = ((float)rand() / (float)RAND_MAX) - 0.5f;
        float rand_u = ((float)rand() / (float)RAND_MAX) - 0.5f;

        vec3s offset = vecAdd(vecSca(right, rand_r * jitter_mag), vecSca(up, rand_u * jitter_mag));
        r->points[i] = vecAdd(base_pos, offset);
    }
}

void Sol_Ribbon_Draw(World *world, double dt, Ribbon *r)
{
    float t     = r->_elapsed / r->lifespan;
    float scale = r->thickness * EvaluateCurve(r->scale_curve, 1.0f - t);
    float alpha =  EvaluateCurve(r->alpha_curve, 1.0f - t);

    vec4s color = r->color;
    color.w *= alpha;

    // How many world units one full texture repeat should cover
    const float units_per_repeat = 5.0f;
    float current_distance       = 0.0f;

    for (int p = 1; p < r->_point_count; p++)
    {
        vec3s pA = r->points[p - 1];
        vec3s pB = r->points[p];

        // 1. Get neighbors (fallback to current point at the extreme ends)
        vec3s pPrev = (p > 1) ? r->points[p - 2] : pA;
        vec3s pNext = (p < r->_point_count - 1) ? r->points[p + 1] : pB;

        // 2. The shared tangent is the vector from previous to next
        vec3s dirA = vecNorm(vecSub(pB, pPrev));
        vec3s dirB = vecNorm(vecSub(pNext, pA));

        vec4s posA4 = {pA.x, pA.y, pA.z, scale};
        vec4s posB4 = {pB.x, pB.y, pB.z, scale};

        float segment_length = glms_vec3_distance(pA, pB);
        float u_start        = current_distance / units_per_repeat;
        float u_end          = (current_distance + segment_length) / units_per_repeat;

        *Sol_Render_GetNext_RibbonSeg(PIPE_RIBBON) = (RibbonSegSSBO){
            .posA      = posA4,
            .posB      = posB4,
            .dirA      = (vec4s){dirA.x, dirA.y, dirA.z, 0.0f}, // Pass to shader
            .dirB      = (vec4s){dirB.x, dirB.y, dirB.z, 0.0f}, // Pass to shader
            .panSpeed  = r->pan,
            .colorA    = color,
            .colorB    = color,
            .textureId = r->texture,
            .uv        = {u_end - u_start, 1.0f, u_start, 0.0f},
        };

        current_distance += segment_length;
    }
}

void Sol_Draw_Lightning(World *world, double dt, Ribbon *r)
{
    GenerateLightningPoints(r);
    Sol_Ribbon_Draw(world, dt, r);
}

typedef void (*DrawRibbon)(World *world, double dt, Ribbon *r);
const DrawRibbon ribbon_draw[RIBBONKIND_COUNT] = {
    [RIBBONKIND_BASIC]     = Sol_Ribbon_Draw,
    [RIBBONKIND_LIGHTNING] = Sol_Draw_Lightning,
};

void Ribbon_Update(World *world, double dt)
{
    SlRibbon *sl = Sol_Comp_Get(world, 0, SlRibbon);
    int write    = 0;
    for (int i = 0; i < solb_count(sl->ribbons); i++)
    {
        Ribbon *r = &sl->ribbons[i];
        r->_elapsed += dt;
        if (r->_elapsed >= r->lifespan)
            continue;

        DrawRibbon draw = ribbon_draw[r->kind];
        if (draw)
            draw(world, dt, r);

        sl->ribbons[write++] = *r;
    }
    solb_set_count(sl->ribbons, write);

    SparseSet_ScRibbon *set = Sol_Comp_Set(world, ScRibbon);
    for (int i = 0; i < set->cnt; i++)
    {
        int id       = set->dense[i];
        ScRibbon *sc = &set->data[i];
        int write    = 0;
        for (int c = 0; c < solb_count(sc->ribbons); c++)
        {
            Ribbon *r = &sc->ribbons[c];
            r->_elapsed += dt;
            if (r->_elapsed >= r->lifespan)
                continue;

            DrawRibbon draw = ribbon_draw[r->kind];
            if (draw)
                draw(world, dt, r);
            sc->ribbons[write++] = *r;
        }
        solb_set_count(sc->ribbons, write);
    }
}

void Sol_Ribbon_Spawn(World *world, RibbonKind kind, vec3s posA, vec3s posB)
{
    SlRibbon *sl   = Sol_Comp_Get(world, 0, SlRibbon);
    Ribbon r       = ribbon_kinds[kind];
    r.kind         = kind;
    r.points[0]    = posA;
    r.points[1]    = posB;
    r._point_count = 2;

    solb_push(sl->ribbons, r);
}

void Sol_Ribbon_SpawnE(World *world, RibbonKind kind, u32 entA, u32 entB)
{
    SlRibbon *sl   = Sol_Comp_Get(world, 0, SlRibbon);
    Ribbon r       = ribbon_kinds[kind];
    r.kind         = kind;
    r.entA = entA;
    r.entB = entB;

    r.points[0]    = world->xform.pos[entA];
    r.points[1]    = world->xform.pos[entB];
    r._point_count = 2;

    solb_push(sl->ribbons, r);
}

Ribbon *Sol_Ribbon_Next(World *world)
{
    SlRibbon *sl = Sol_Comp_Get(world, 0, SlRibbon);
    Ribbon *r    = solb_next(sl->ribbons);
    *r           = (Ribbon){0};
    return r;
}
