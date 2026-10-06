#include "world.h"
#include "sol_math.h"
#include "render/render.h"

Ribbon ribbon_kinds[RIBBONKIND_COUNT] = {
    [RIBBONKIND_BASIC] =
        {
            .color       = {1.0f, 1.0f, 1.0f, 1.0f},
            .lifespan    = 5.0f,
            .scale_curve = CURVE_LINEAR_FADEOUT,
            .alpha_curve = CURVE_LINEAR_FADEOUT,
            .texture     = SOL_TEXTURE_BEAM,
            .thickness   = 1.0f,
            .pan         = 1.0f,
        },
    [RIBBONKIND_LIGHTNING] =
        {
            .color       = {1.0f, 1.0f, 1.0f, 1.0f},
            .lifespan    = 0.3f,
            .scale_curve = CURVE_LINEAR_FADEOUT,
            .alpha_curve = CURVE_LINEAR_FADEOUT,
            .texture     = SOL_TEXTURE_LIGHTNING,
            .thickness   = 1.0f,
            .pan         = 2.0f,
        },
    [RIBBONKIND_LIGHTNING_WEAPON_TRAIL] =
        {
            .color       = {1.0f, 1.0f, 1.0f, 1.0f},
            .lifespan    = 0.2f,
            .scale_curve = CURVE_LINEAR_FADEOUT,
            .alpha_curve = CURVE_LINEAR_FADEOUT,
            .texture     = SOL_TEXTURE_BEAM,
            .thickness   = 1.0f,
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

static void Ribbon_RemoveFirstSegment(Ribbon *r)
{
    if (r->_point_count <= 1)
    {
        r->_point_count = 0;
        return;
    }

    memmove(&r->points[0], &r->points[1], (size_t)(r->_point_count - 1) * sizeof(r->points[0]));

    memmove(&r->_seg_elapsed[0], &r->_seg_elapsed[1], (size_t)(r->_point_count - 2) * sizeof(r->_seg_elapsed[0]));

    r->_point_count--;
}

void Sol_Ribbon_GenerateJitter(Ribbon *r, float jitter_mag)
{
    vec3s start = r->points[0];
    vec3s end   = r->points[r->_point_count - 1]; // Keep the current target

    r->points[0]                   = start;
    r->points[r->_point_count - 1] = end;

    vec3s dir   = vecSub(end, start);
    vec3s right = vecNorm(vecCross(dir, WORLD_UP));
    if (vecLen(right) < 0.01f)
        right = (vec3s){1.0f, 0, 0}; // Fallback if pointing straight up
    vec3s up = vecNorm(vecCross(right, dir));

    for (int i = 1; i < r->_point_count - 1; i++)
    {
        float t        = (float)i / (float)(r->_point_count - 1);
        vec3s base_pos = vecAdd(start, vecSca(dir, t));

        // Random offset between -0.5 and 0.5, scaled by distance
        float rand_r = ((float)rand() / (float)RAND_MAX) - 0.5f;
        float rand_u = ((float)rand() / (float)RAND_MAX) - 0.5f;

        vec3s offset = vecAdd(vecSca(right, rand_r * jitter_mag), vecSca(up, rand_u * jitter_mag));
        r->points[i] = vecAdd(base_pos, offset);
    }
}

const float sheet_rot[3] = {0.0f, -45.0f, 45.0f};
bool Sol_Ribbon_Draw(World *world, double dt, Ribbon *r)
{
    // How many world units one full texture repeat should cover
    const float units_per_repeat = 5.0f;
    float current_distance       = 0.0f;
    for (int sheets = 0; sheets < r->sheets + 1; sheets++)
    {
        for (int p = 1; p < r->_point_count; p++)
        {
            r->_seg_elapsed[p - 1] += dt;
            float t     = r->_seg_elapsed[p - 1] / r->lifespan;
            float scale = r->thickness * EvaluateCurve(r->scale_curve, t);
            float alpha = EvaluateCurve(r->alpha_curve, t);
            vec4s color = r->color;
            color.w *= alpha;

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
                .spin      = r->spin[p] + sheet_rot[sheets],
                .panSpeed  = r->pan,
                .colorA    = color,
                .colorB    = color,
                .flags     = r->flags,
                .textureId = r->texture,
                .uv        = {u_end - u_start, 1.0f, u_start, 0.0f},
            };

            current_distance += segment_length;
        }
    }
    if (r->_point_count <= 1)
        return false;
    return true;
}

static const bool (*ribbon_draw[RIBBONKIND_COUNT])(World *world, double dt, Ribbon *r) = {
    [RIBBONKIND_BASIC]                  = Sol_Ribbon_Draw,
    [RIBBONKIND_LIGHTNING]              = Sol_Ribbon_Draw,
    [RIBBONKIND_LIGHTNING_WEAPON_TRAIL] = Sol_Ribbon_Draw,
};

void Ribbon_Update(World *world, double dt)
{
    SlRibbon *sl = Sol_Comp_Get(world, 0, SlRibbon);
    int write    = 0;
    for (int i = 0; i < solb_count(sl->ribbons); i++)
    {
        Ribbon *r = &sl->ribbons[i];
        ribbon_draw[r->kind](world, dt, r);
        while (r->lifespan > 0 && r->_point_count > 1 && r->_seg_elapsed[0] >= r->lifespan)
            Ribbon_RemoveFirstSegment(r);

        if (r->_point_count > 1)
            sl->ribbons[write++] = *r;
    }
    solb_set_count(sl->ribbons, write);

    SparseSet_ScRibbon *set = Sol_Comp_Set(world, ScRibbon);
    for (int i = 0; i < set->cnt; i++)
    {
        int id       = set->dense[i];
        ScRibbon *sc = &set->data[i];
        int write    = 0;
        Ribbon *r    = &sc->ribbon;
        ribbon_draw[r->kind](world, dt, r);
        while (r->lifespan > 0 && r->_point_count > 1 && r->_seg_elapsed[0] >= r->lifespan)
            Ribbon_RemoveFirstSegment(r);
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
    SlRibbon *sl = Sol_Comp_Get(world, 0, SlRibbon);
    Ribbon r     = ribbon_kinds[kind];
    r.kind       = kind;
    r.entA       = entA;
    r.entB       = entB;

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

ScRibbon *Sol_Ribbon_AddKind(World *world, int id, RibbonKind kind)
{
    Ribbon r         = ribbon_kinds[kind];
    ScRibbon *ribbon = Sol_Comp_Add(world, id, ScRibbon);
    ribbon->ribbon   = r;

    return ribbon;
}

void Sol_Ribbon_Addpoint(Ribbon *r, vec3s pos, float jitter_mag, float spin)
{
    if (r->_point_count >= MAX_RIBBON_SEGMENTS)
        Ribbon_RemoveFirstSegment(r);

    int p        = r->_point_count++;
    r->points[p] = pos;
    r->spin[p]   = spin;
    if (p > 0)
    {
        r->_seg_elapsed[p - 1] = 0.0f;

        if (jitter_mag > 0)
        {
            vec3s start = r->points[p - 1];
            vec3s end   = r->points[p]; // Keep the current target

            vec3s dir   = vecSub(end, start);
            vec3s right = vecNorm(vecCross(dir, WORLD_UP));
            if (vecLen(right) < 0.01f)
                right = (vec3s){1.0f, 0, 0}; // Fallback if pointing straight up
            vec3s up = vecNorm(vecCross(right, dir));

            // Random offset between -0.5 and 0.5, scaled by distance
            float rand_r = ((float)rand() / (float)RAND_MAX) - 0.5f;
            float rand_u = ((float)rand() / (float)RAND_MAX) - 0.5f;

            vec3s offset = vecAdd(vecSca(right, rand_r * jitter_mag), vecSca(up, rand_u * jitter_mag));
            r->points[p] = vecAdd(r->points[p], offset);
        }
    }
}
