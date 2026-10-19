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
    [RIBBONKIND_FIRE] =
        {
            .color       = {1.0f, 0.0f, 0.0f, 1.0f},
            .lifespan    = 1.0f,
            .scale_curve = CURVE_LINEAR_FADEOUT,
            .alpha_curve = CURVE_LINEAR_FADEOUT,
            .texture     = SOL_TEXTURE_FOGSTRIP,
            .thickness   = 0.8f,
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
    [RIBBONKIND_WEAPON_TRAIL] =
        {
            .color       = {1.0f, 1.0f, 1.0f, 1.0f},
            .lifespan    = 0.4f,
            .alpha_curve = CURVE_LINEAR_FADEOUT,
            .texture     = SOL_TEXTURE_WEAPONTRAIL,
            .flags       = 1,
            .thickness   = 1.0f,
        },
};

#define UNITS_PER_REPEAT 5.0f

enum
{
    RIBBONFLAG_AXIS = (1u << 0),
};

typedef struct
{
    float width[MAX_RIBBON_SEGMENTS];
    float alpha[MAX_RIBBON_SEGMENTS];
    float dist[MAX_RIBBON_SEGMENTS];
} RibbonPointData;

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

// Direction from -> to, with a fallback for coincident points (avoids NaNs)
static vec3s Ribbon_Dir(vec3s from, vec3s to, vec3s fallback)
{
    vec3s d = vecSub(to, from);
    float l = vecLen(d);
    return (l > 1e-6f) ? vecSca(d, 1.0f / l) : fallback;
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
static void Ribbon_EmitSegments(const Ribbon *r, u32 flags, const vec3s *axis, const RibbonPointData *pd)
{
    for (int p = 1; p < r->_point_count; p++)
    {
        vec3s pA = r->points[p - 1], pB = r->points[p];
        float dA = pd->dist[p - 1], dB = pd->dist[p];

        // Head trim: shrink the oldest segment toward its newer end as it fades.
        // Distance is trimmed too so the texture doesn't stretch.
        if (p == 1)
        {
            pA = vecAdd(pB, vecSca(vecSub(pA, pB), pd->alpha[0]));
            dA = dB - (dB - dA) * pd->alpha[0];
        }

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
            .flags     = flags,
            .textureId = r->texture,
            .panSpeed  = r->pan,
            .uv        = {u1 - u0, 1.0f, u0, 0.0f},
        };
    }
}

// Camera-facing ribbon: axis = path tangent, shader builds side from the view.
bool Sol_Ribbon_Draw(World *world, double dt, Ribbon *r)
{
    const int n = r->_point_count;
    if (n <= 1)
        return false;

    RibbonPointData pd;
    Ribbon_BuildPointData(r, &pd);

    // One tangent per point, shared by both neighbouring segments
    vec3s tangent[MAX_RIBBON_SEGMENTS];
    for (int i = 0; i < n; i++)
    {
        vec3s fallback = (i > 0) ? tangent[i - 1] : WORLD_FWD;
        tangent[i]     = Ribbon_Dir(r->points[i > 0 ? i - 1 : i], r->points[i < n - 1 ? i + 1 : i], fallback);
    }

    // Miter compensation so bends don't pinch
    for (int i = 0; i < n; i++)
    {
        vec3s d0 = (i > 0) ? Ribbon_Dir(r->points[i - 1], r->points[i], tangent[i]) : tangent[i];
        vec3s d1 = (i < n - 1) ? Ribbon_Dir(r->points[i], r->points[i + 1], tangent[i]) : tangent[i];
        float c  = fminf(fmaxf(glms_vec3_dot(tangent[i], d0), 0.0f), fmaxf(glms_vec3_dot(tangent[i], d1), 0.0f));
        pd.width[i] /= fmaxf(c, 0.3f);
    }

    Ribbon_EmitSegments(r, r->flags & ~RIBBONFLAG_AXIS, tangent, &pd);
    return true;
}

// Weapon trail: side[] (blade axis from the bone) is the width direction.
// No tangents, no miter, nothing to derive.
bool Sol_Ribbon_Draw_WeaponTrail(World *world, double dt, Ribbon *r)
{
    if (r->_point_count <= 1)
        return false;

    RibbonPointData pd;
    Ribbon_BuildPointData(r, &pd);

    Ribbon_EmitSegments(r, r->flags | RIBBONFLAG_AXIS, r->side, &pd);
    return true;
}

static const bool (*ribbon_draw[RIBBONKIND_COUNT])(World *world, double dt, Ribbon *r) = {
    [RIBBONKIND_BASIC]        = Sol_Ribbon_Draw,
    [RIBBONKIND_LIGHTNING]    = Sol_Ribbon_Draw,
    [RIBBONKIND_WEAPON_TRAIL] = Sol_Ribbon_Draw_WeaponTrail,
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

        if (ribbon_draw[r->kind])
            ribbon_draw[r->kind](world, dt, r);
        else
            Sol_Ribbon_Draw(world, dt, r);

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
        for (int p = 0; p < r->_point_count; p++)
            r->_point_elapsed[p] += dt;
        while (r->lifespan > 0 && r->_point_count > 1 && r->_point_elapsed[0] >= r->lifespan)
            Ribbon_RemoveFirstSegment(r);

        if (ribbon_draw[r->kind])
            ribbon_draw[r->kind](world, dt, r);
        else
            Sol_Ribbon_Draw(world, dt, r);
    }
}

void Sol_Ribbon_Spawn(World *world, RibbonKind kind, vec3s posA, vec3s posB)
{
    SlRibbon *sl = Sol_Comp_Get(world, 0, SlRibbon);
    Ribbon r     = ribbon_kinds[kind];
    r.kind       = kind;
    Sol_Ribbon_Addpoint(&r, posA, WORLD_UP);
    Sol_Ribbon_Addpoint(&r, posB, WORLD_UP);
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
    r.kind           = kind;
    ScRibbon *ribbon = Sol_Comp_Add(world, id, ScRibbon);
    ribbon->ribbon   = r;
    return ribbon;
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