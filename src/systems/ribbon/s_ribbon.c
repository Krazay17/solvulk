/*
 * File: s_ribbon.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-10-19
 *
 */
#include "s_ribbon.h"
#include "world.h"
#include "sol_math.h"
#include "render/render.h"

Ribbon ribbon_kinds[RIBBONKIND_COUNT] = {
    [RIBBONKIND_BASIC] =
        {
            .color        = {1.0f, 1.0f, 1.0f, 1.0f},
            .lifespan     = 5.0f,
            .scale_curve  = CURVE_LINEAR_FADEOUT,
            .alpha_curve  = CURVE_LINEAR_FADEOUT,
            .texture      = SOL_TEXTURE_BEAM,
            .thickness[0] = 1.0f,
            .pan          = 1.0f,
        },
    [RIBBONKIND_FIRE] =
        {
            .color        = {1.0f, 0.0f, 0.0f, 1.0f},
            .lifespan     = 1.0f,
            .scale_curve  = CURVE_LINEAR_FADEOUT,
            .alpha_curve  = CURVE_LINEAR_FADEOUT,
            .texture      = SOL_TEXTURE_FOGSTRIP,
            .thickness[0] = 0.8f,
            .pan          = 1.0f,
        },
    [RIBBONKIND_LIGHTNING] =
        {
            .color        = {1.0f, 1.0f, 1.0f, 1.0f},
            .lifespan     = 0.3f,
            .scale_curve  = CURVE_LINEAR_FADEOUT,
            .alpha_curve  = CURVE_LINEAR_FADEOUT,
            .texture      = SOL_TEXTURE_LIGHTNING,
            .thickness[0] = 1.0f,
            .pan          = 2.0f,
        },
    [RIBBONKIND_WEAPON_TRAIL] =
        {
            .color        = {1.0f, 1.0f, 1.0f, 1.0f},
            .lifespan     = 0.4f,
            .alpha_curve  = CURVE_LINEAR_FADEOUT,
            .texture      = SOL_TEXTURE_WEAPONTRAIL,
            .thickness[0] = 1.0f,
            .flags        = RIBBONFLAG_NOFACECAM,
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

#define UNITS_PER_REPEAT 5.0f

typedef struct
{
    float width[MAX_RIBBON_SEGMENTS];
    float alpha[MAX_RIBBON_SEGMENTS];
    float dist[MAX_RIBBON_SEGMENTS];
} RibbonPointData;

static void Ribbon_RemoveFirstSegment(Ribbon *r)
{
    if (r->_point_count <= 1)
    {
        r->_point_count = 0;
        r->_dist_origin = 0.0f;
        return;
    }
    int n = r->_point_count - 1;
    r->_dist_origin += glms_vec3_distance(r->points[0], r->points[1]);

    memmove(&r->points[0], &r->points[1], n * sizeof(r->points[0]));
    memmove(&r->side[0], &r->side[1], n * sizeof(r->side[0]));
    memmove(&r->_point_elapsed[0], &r->_point_elapsed[1], n * sizeof(r->_point_elapsed[0]));

    if (r->nonuniform_thickness)
        memmove(&r->thickness[0], &r->thickness[1], n * sizeof(r->thickness[0]));
    r->_point_count--;
}
static void GenerateJitter(Ribbon *r, float jitter_mag)
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

// Direction from -> to, with a fallback for coincident points (avoids NaNs)
static vec3s Ribbon_Dir(vec3s from, vec3s to, vec3s fallback)
{
    vec3s d = vecSub(to, from);
    float l = vecLen(d);
    return (l > 1e-6f) ? vecSca(d, 1.0f / l) : fallback;
}

// Project side into the plane perpendicular to the path, then roll about the path.
static vec3s RollSide(vec3s side, vec3s t, float rad)
{
    vec3s v = vecSub(side, vecSca(t, glms_vec3_dot(side, t)));
    float l = vecLen(v);
    if (l < 1e-4f)
        return side;
    v = vecSca(v, 1.0f / l);
    return vecAdd(vecSca(v, cosf(rad)), vecSca(vecCross(t, v), sinf(rad)));
}

static void Ribbon_BuildTangents(const Ribbon *r, vec3s *tangent)
{
    const int n = r->_point_count;
    for (int i = 0; i < n; i++)
    {
        vec3s fallback = (i > 0) ? tangent[i - 1] : WORLD_FWD;
        tangent[i]     = Ribbon_Dir(r->points[i > 0 ? i - 1 : i], r->points[i < n - 1 ? i + 1 : i], fallback);
    }
}

static void Ribbon_ApplyMiter(const Ribbon *r, const vec3s *tangent, RibbonPointData *pd)
{
    const int n = r->_point_count;
    for (int i = 0; i < n; i++)
    {
        vec3s d0 = (i > 0) ? Ribbon_Dir(r->points[i - 1], r->points[i], tangent[i]) : tangent[i];
        vec3s d1 = (i < n - 1) ? Ribbon_Dir(r->points[i], r->points[i + 1], tangent[i]) : tangent[i];
        float c  = fminf(fmaxf(glms_vec3_dot(tangent[i], d0), 0.0f), fmaxf(glms_vec3_dot(tangent[i], d1), 0.0f));
        pd->width[i] /= fmaxf(c, 0.3f);
    }
}
// Per-point width, alpha and path distance (shared by every ribbon type)
static void Ribbon_BuildPointData(const Ribbon *r, RibbonPointData *pd)
{
    const int n = r->_point_count;
    for (int i = 0; i < n; i++)
    {
        pd->dist[i] = (i == 0) ? r->_dist_origin : pd->dist[i - 1] + glms_vec3_distance(r->points[i - 1], r->points[i]);

        float t      = (r->lifespan > 0.0f) ? r->_point_elapsed[i] / r->lifespan : 0.0f;
        float th     = r->nonuniform_thickness ? r->thickness[i] : r->thickness[0];
        pd->width[i] = th * EvaluateCurve(r->scale_curve, t);
        pd->alpha[i] = EvaluateCurve(r->alpha_curve, t);
    }
}

// Writes one SSBO segment per point pair. `axis[i]` is whatever the shader
// expects for these flags (tangent for camera-facing, blade axis for AXIS).
static void Ribbon_EmitSegments(const Ribbon *r, const vec3s *axis, const RibbonPointData *pd)
{
    for (int p = 1; p < r->_point_count; p++)
    {
        vec3s pA = r->points[p - 1], pB = r->points[p];
        float dA = pd->dist[p - 1], dB = pd->dist[p];

        // Head trim: shrink the oldest segment toward its newer end as it fades.
        // Distance is trimmed too so the texture doesn't stretch.
        // if (p == 1)
        // {
        //     pA = vecAdd(pB, vecSca(vecSub(pA, pB), pd->alpha[0]));
        //     dA = dB - (dB - dA) * pd->alpha[0];
        // }

        vec4s cA = r->color, cB = r->color;
        cA.w *= pd->alpha[p - 1];
        cB.w *= pd->alpha[p];

        float u0 = dA / UNITS_PER_REPEAT;
        float u1 = dB / UNITS_PER_REPEAT;

        *Sol_Render_GetNext_RibbonSeg(PIPE_RIBBON) = (RibbonSegSSBO){
            .posA      = {pA.x, pA.y, pA.z, pd->width[p - 1]},
            .posB      = {pB.x, pB.y, pB.z, pd->width[p]},
            .axisA     = {axis[p - 1].x, axis[p - 1].y, axis[p - 1].z, 0.0f},
            .axisB     = {axis[p].x, axis[p].y, axis[p].z, 0.0f},
            .colorA    = cA,
            .colorB    = cB,
            .flags     = r->flags,
            .textureId = r->texture,
            .panSpeed  = r->pan,
            .uv        = {u1 - u0, 1.0f, u0, 0.0f},
        };
    }
}

static void Draw(World *world, double dt, Ribbon *r)
{
    const int n = r->_point_count;
    if (n <= 1)
        return;

    RibbonPointData pd;
    Ribbon_BuildPointData(r, &pd);

    vec3s tangent[MAX_RIBBON_SEGMENTS];

    // Camera-facing: one sheet, tangent is the axis
    if (!(r->flags & RIBBONFLAG_NOFACECAM))
    {
        Ribbon_BuildTangents(r, tangent);
        Ribbon_ApplyMiter(r, tangent, &pd);
        Ribbon_EmitSegments(r, tangent, &pd);
        return;
    }

    // Blade mode: sheet 0 uses the stored axis as-is
    Ribbon_EmitSegments(r, r->side, &pd);
    if (r->sheets == 0)
        return;

    // Extra sheets: roll the original side[] about the tangent
    Ribbon_BuildTangents(r, tangent);

    vec3s rolled[MAX_RIBBON_SEGMENTS];
    const int total = (int)r->sheets + 1;
    for (int s = 1; s < total; s++)
    {
        float roll = GLM_PIf * (float)s / (float)total; // even spacing over 180°
        for (int i = 0; i < n; i++)
            rolled[i] = RollSide(r->side[i], tangent[i], roll);
        Ribbon_EmitSegments(r, rolled, &pd);
    }
}

static void Draw_Lightning(World *world, double dt, Ribbon *r)
{
    GenerateJitter(r, 0.33f);
    Draw(world, dt, r);
}

void (*const ribbon_draw_kind[RIBBONKIND_COUNT])(World *world, double dt, Ribbon *r) = {
    [RIBBONKIND_BASIC]        = Draw,
    [RIBBONKIND_WEAPON_TRAIL] = Draw,
    [RIBBONKIND_LIGHTNING]    = Draw_Lightning,
};

void Ribbon_Update(World *world, double dt)
{
    SlRibbon *sl = Sol_Comp_Get(world, 0, SlRibbon);
    int write    = 0;
    for (int i = 0; i < solb_count(sl->ribbons); i++)
    {
        Ribbon *r = &sl->ribbons[i];
        for (int p = 0; p < r->_point_count; p++)
            r->_point_elapsed[p] += dt;
        while (r->lifespan > 0 && r->_point_count > 1 && r->_point_elapsed[0] >= r->lifespan)
            Ribbon_RemoveFirstSegment(r);

        ribbon_draw_kind[r->kind] ? ribbon_draw_kind[r->kind](world, dt, r) : Draw(world, dt, r);

        if (r->_point_count > 1)
        {
            if (write != i)
                sl->ribbons[write] = *r;
            write++;
        }
    }
    solb_set_count(sl->ribbons, write);

    SparseSet_ScRibbon *set = Sol_Comp_Set(world, ScRibbon);
    for (int i = 0; i < set->cnt; i++)
    {
        int id       = set->dense[i];
        ScRibbon *sc = &set->data[i];
        for (int j = 0; j < solb_count(sc->ribbons); j++)
        {
            Ribbon *r = &sc->ribbons[j];
            for (int p = 0; p < r->_point_count; p++)
                r->_point_elapsed[p] += dt;
            while (r->lifespan > 0 && r->_point_count > 1 && r->_point_elapsed[0] >= r->lifespan)
                Ribbon_RemoveFirstSegment(r);

            if (r->rate > 0)
            {
                r->_accum += dt;
                while (r->_accum >= r->rate)
                {
                    r->_accum -= r->rate;
                    Sol_Ribbon_Addpoint(r, world->xform.draw_pos[id], WORLD_UP);
                }
            }

            ribbon_draw_kind[r->kind] ? ribbon_draw_kind[r->kind](world, dt, r) : Draw(world, dt, r);
        }
    }
}

Ribbon Ribbon_Make(RibbonKind kind)
{
    Ribbon r = ribbon_kinds[kind];
    r.kind   = kind;
    return r;
}

Ribbon *Sol_Ribbon_AddKind(World *world, int id, RibbonKind kind)
{
    ScRibbon *sc = Sol_Comp_Add(world, id, ScRibbon);
    Ribbon *r    = solb_next(sc->ribbons);
    *r           = Ribbon_Make(kind);
    return r;
}

Ribbon *Sol_Ribbon_Next(World *world, RibbonKind kind)
{
    SlRibbon *sl = Sol_Comp_Get(world, 0, SlRibbon);
    Ribbon *r    = solb_next(sl->ribbons);
    *r           = Ribbon_Make(kind);
    return r;
}

void Sol_Ribbon_Spawn(World *world, RibbonKind kind, vec3s posA, vec3s posB)
{
    SlRibbon *sl = Sol_Comp_Get(world, 0, SlRibbon);
    Ribbon r     = Ribbon_Make(kind);
    Sol_Ribbon_Addpoint(&r, posA, WORLD_UP);
    Sol_Ribbon_Addpoint(&r, posB, WORLD_UP);

    solb_push(sl->ribbons, r);
}

void Sol_Ribbon_SpawnE(World *world, RibbonKind kind, u32 entA, u32 entB)
{
    SlRibbon *sl = Sol_Comp_Get(world, 0, SlRibbon);
    Ribbon r     = Ribbon_Make(kind);
    r.entA       = entA;
    r.entB       = entB;

    Sol_Ribbon_Addpoint(&r, world->xform.pos[entA], WORLD_UP);
    Sol_Ribbon_Addpoint(&r, world->xform.pos[entB], WORLD_UP);

    solb_push(sl->ribbons, r);
}

Ribbon *Sol_Ribbon_Find(ScRibbon *sc, RibbonKind kind)
{
    for (int i = 0; i < solb_count(sc->ribbons); i++)
    {
        Ribbon *r = &sc->ribbons[i];
        if (r->kind == kind)
            return r;
    }
    return NULL;
}

bool Sol_Ribbon_Due(Ribbon *r, float dt, float interval)
{
    r->_accum += dt;
    if (r->_accum < interval)
        return false;
    r->_accum -= interval;
    if (r->_accum > interval)
        r->_accum = 0;
    return true;
}

// return current point idx
int Sol_Ribbon_Addpoint(Ribbon *r, vec3s pos, vec3s side)
{
    if (r->_point_count >= MAX_RIBBON_SEGMENTS)
        Ribbon_RemoveFirstSegment(r);

    int p                = r->_point_count++;
    r->points[p]         = pos;
    r->side[p]           = side;
    r->_point_elapsed[p] = 0.0f;

    return p;
}

void Sol_Ribbon_Addpoint_Jitter(Ribbon *r, vec3s pos, vec3s side, float jitter_mag)
{
    int p = Sol_Ribbon_Addpoint(r, pos, side);

    if (jitter_mag <= 0 || p == 0)
        return;

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