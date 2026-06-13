/*
 * File: s_ribbon.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-06-10
 *
 */
#include "sol_core.h"

typedef struct SolRibbons
{
    Ribbon *ribbon;
    u32     ribbon_count;
    u32     ribbon_capacity;
} SolRibbons;

static void Ribbon_Step(World *world, double dt, double time);
static void Ribbon_Draw(World *world, double dt, double time);

static void Ribbon_AppendPoint(Ribbon *r, vec3s pos);
static void Ribbon_Update_One(Ribbon *r, float fdt);

static void Ribbon_Update_Trail(World *world, Ribbon *r, double dt, double time);
static void Ribbon_Update_Beam(World *world, Ribbon *r, double dt, double time);
static void Ribbon_Draw_Lightning(World *world, Ribbon *r, double dt, double time);
static void Ribbon_Draw_Laser(World *world, Ribbon *r, double dt, double time);
typedef void (*RibbonFunc)(World *, Ribbon *, double, double);

typedef struct
{
    float      ttl;
    float      width;
    float      segLifetime;
    float      rate;
    int        segments;
    float      stretch;
    vec4s      color;
    u8         inf;
    RibbonAttachMode attachMode;
    RibbonFunc step, draw;
} RibbonConfig;

static const RibbonConfig ribbon_config[RIBBONKIND_COUNT] = {
    [RIBBONKIND_BULLETTRAIL] = {
        .inf         = 1,
        .width       = 0.05f,
        .segLifetime = 0.15f,
        .rate        = 0.02f,
        .segments    = 16,
        .stretch     = 1.0f,
        .color       = {1, 0.8f, 0.4f, 1},
        .attachMode  = RIBBONATTACH_TRAIL,
        .step        = Ribbon_Update_Trail,
        .draw        = Ribbon_Draw_Laser,
    },
    [RIBBONKIND_LIGHTNING] = {
        .ttl         = 0.2f,
        .width       = 0.12f,
        .segLifetime = 9999.f,  // beams never age out per-segment
        .segments    = 16,
        .stretch     = 1.0f,
        .color       = {0.6f, 0.8f, 1.0f, 1.0f},
        .attachMode  = RIBBONATTACH_ENT_TO_ENT,
        .step        = Ribbon_Update_Beam,
        .draw        = Ribbon_Draw_Lightning,
    },
    [RIBBONKIND_LASER] = {
        .inf         = 1,
        .width       = 0.08f,
        .segLifetime = 9999.f,
        .segments    = 2,       // laser is just one segment, no subdivisions needed
        .stretch     = 4.0f,
        .color       = {1, 0.2f, 0.2f, 1},
        .attachMode  = RIBBONATTACH_ENT_TO_POS,
        .step        = Ribbon_Update_Beam,
        .draw        = Ribbon_Draw_Laser,
    },
};

void Sol_Ribbon_Init(World *world)
{
    world->ribbons = malloc(sizeof(SolRibbons));

    world->ribbons->ribbon_count    = 0;
    world->ribbons->ribbon_capacity = MAX_RIBBONS;
    world->ribbons->ribbon          = calloc(MAX_RIBBONS, sizeof(Ribbon));

    WAddStep(world) = Ribbon_Step;
    WAdd3d(world)   = Ribbon_Draw;
}

Ribbon *Sol_Ribbon_Spawn(World *world, RibbonKind kind, vec3s pos, vec3s posB, vec4s color)
{
    SolRibbons *s = world->ribbons;
    Sol_Realloc(&s->ribbon, s->ribbon_count, &s->ribbon_capacity, sizeof(Ribbon));

    const RibbonConfig *cfg = &ribbon_config[kind];

    Ribbon *r = &s->ribbon[s->ribbon_count++];
    *r = (Ribbon){
        .kind        = kind,
        .inf         = cfg->inf,
        .ttl         = cfg->ttl,
        .width       = cfg->width,
        .segLifetime = cfg->segLifetime,
        .rate        = cfg->rate,
        .segments    = cfg->segments > 0 ? cfg->segments : MAX_RIBBON_SEGS,
        .stretch     = cfg->stretch,
        .attachMode  = cfg->attachMode,
        .color       = color.a > 0 ? color : cfg->color,
        .targetPos   = posB,
    };

    Ribbon_AppendPoint(r, pos);
    return r;
}

Ribbon *Sol_Ribbon_Add(World *world, int id, RibbonKind kind, float width, vec4s color)
{
    vec3s pos = Sol_Xform_GetPos(world, id);
    Ribbon *r = Sol_Ribbon_Spawn(world, kind, pos, (vec3s){0}, color);
    r->followId = id;
    if (width > 0) r->width = width;
    return r;
}

Ribbon *Sol_Ribbon_AddBetweenEntities(World *world, int entA, int entB, RibbonKind kind, float width, vec4s color)
{
    Ribbon *r   = Sol_Ribbon_Spawn(world, kind, (vec3s){0}, (vec3s){0}, color);
    r->followId = entA;
    r->targetId = entB;
    r->attachMode = RIBBONATTACH_ENT_TO_ENT; // override even if config defaulted differently
    if (width > 0) r->width = width;
    return r;
}

// ################
// --- internal ---
// ################
static void Ribbon_Step(World *world, double dt, double time)
{
    SolRibbons *s = world->ribbons;
    if (!s) return;
    float fdt = (float)dt;
    int write = 0;

    for (int i = 0; i < s->ribbon_count; i++)
    {
        Ribbon *r = &s->ribbon[i];

        // Detach if the followed entity died
        if (r->followId && !(world->masks[r->followId] & HAS_ACTIVE))
        {
            r->followId = 0;
            r->targetId = 0;
            r->inf      = false;
            //r->ttl      = r->segLifetime; // let existing trail fade
        }

        if (!r->inf)
        {
            r->ttl -= fdt;
            if (r->ttl <= 0) continue;
        }

        ribbon_config[r->kind].step(world, r, dt, time);
        s->ribbon[write++] = *r;
    }
    s->ribbon_count = write;
}

static void Ribbon_Draw(World *world, double dt, double time)
{
    SolRibbons *s = world->ribbons;
    for (int i = 0; i < s->ribbon_count; i++)
    {
        ribbon_config[s->ribbon[i].kind].draw(world, &s->ribbon[i], dt, time);
    }
}

static void Ribbon_Update_Trail(World *world, Ribbon *r, double dt, double time)
{
    float fdt = (float)dt;
    vec3s pos = Sol_Xform_GetDrawXform(world, r->followId).pos;

    r->accumulator += vecDist(pos, r->points[r->head]);
    if (r->rate == 0 || r->accumulator >= r->rate)
    {
        Ribbon_AppendPoint(r, pos);
        r->accumulator = 0;
    }
    Ribbon_Update_One(r, fdt);
}

static void Ribbon_AppendPoint(Ribbon *r, vec3s pos)
{
    r->head            = (r->head + 1) % r->segments;
    r->points[r->head] = pos;
    r->ages[r->head]   = 0.0f;
    if (r->count < r->segments)
        r->count++;
}

static void Ribbon_Update_One(Ribbon *r, float fdt)
{
    // Age all segments; compact by trimming the tail
    int live = 0;
    for (int i = 0; i < r->count; i++)
    {
        int idx = ((r->head - i) % r->segments + r->segments) % r->segments;
        r->ages[idx] += fdt;
        if (r->ages[idx] < r->segLifetime)
            live++;
    }
    // Oldest segments are at the tail; trim count down to live
    // (they'll naturally be skipped in draw by age check)
    r->count = live < r->count ? live + 1 : r->count; // keep at least head
}

static void Ribbon_Update_Beam(World *world, Ribbon *r, double dt, double time)
{
    if (!r->followId)
        return;

    vec3s startPos = Sol_Xform_GetDrawXform(world, r->followId).pos;
    vec3s endPos   = r->targetPos;

    if (r->attachMode == RIBBONATTACH_ENT_TO_ENT)
        endPos = Sol_Xform_GetPos(world, r->targetId);

    r->head  = r->segments - 1;
    r->count = r->segments;

    for (int i = 0; i < r->segments; i++)
    {
        float t      = (float)i / (float)(r->segments - 1);
        r->points[i] = glms_vec3_lerp(startPos, endPos, t);
        r->ages[i]   = 0.0f;
    }
}

static void Ribbon_Draw_Lightning(World *world, Ribbon *r, double dt, double time)
{
    if (r->count < 2)
        return;

    r->uv                = glms_vec2_add(r->uv, glms_vec2_scale(r->uvv, dt));
    r->uv.x              = fmodf(r->uv.x, 1.0f);
    r->uv.y              = fmodf(r->uv.y, 1.0f);
    float invMaxSegments = 1.0f / (float)(r->count - 1);

    for (int i = 0; i < r->count - 1; i++)
    {
        int idxA = ((r->head - i) + r->segments) % r->segments;
        int idxB = ((r->head - i - 1) + r->segments) % r->segments;

        float tA = 1.0f - (r->ages[idxA] / r->segLifetime);
        float tB = 1.0f - (r->ages[idxB] / r->segLifetime);
        tA       = tA < 0 ? 0 : tA;
        tB       = tB < 0 ? 0 : tB;

        RibbonSegSSBO *seg = Sol_Render_GetNext_RibbonSeg(0);
        if (!seg)
            return;

        float startV = (float)i * invMaxSegments;
        float endV   = (float)(i + 1) * invMaxSegments;

        seg->posA   = (vec4s){r->points[idxA].x, r->points[idxA].y, r->points[idxA].z, r->width * tA};
        seg->posB   = (vec4s){r->points[idxB].x, r->points[idxB].y, r->points[idxB].z, r->width * tB};
        seg->colorA = (vec4s){r->color.x, r->color.y, r->color.z, r->color.w * tA};
        seg->colorB = (vec4s){r->color.x, r->color.y, r->color.z, r->color.w * tB};
        seg->uv     = (vec4s){startV * r->stretch, endV * r->stretch, r->uv.x, r->uv.y};
    }
}

static void Ribbon_Draw_Laser(World *world, Ribbon *r, double dt, double time)
{
    if (r->count < 2)
        return;

    r->uv               = glms_vec2_add(r->uv, glms_vec2_scale(r->uvv, dt));
    r->uv.x             = fmodf(r->uv.x, 1.0f);
    r->uv.y             = fmodf(r->uv.y, 1.0f);
    float totalDistance = 0.0f;

    for (int i = 0; i < r->count - 1; i++)
    {
        int idxA = ((r->head - i) + r->segments) % r->segments;
        int idxB = ((r->head - i - 1) + r->segments) % r->segments;

        float tA = 1.0f - (r->ages[idxA] / r->segLifetime);
        float tB = 1.0f - (r->ages[idxB] / r->segLifetime);
        tA       = tA < 0 ? 0 : tA;
        tB       = tB < 0 ? 0 : tB;

        RibbonSegSSBO *seg = Sol_Render_GetNext_RibbonSeg(1);
        if (!seg)
            return;

        float segLength = glms_vec3_distance(r->points[idxA], r->points[idxB]);
        float startV    = totalDistance;
        totalDistance += segLength;
        float endV = totalDistance;

        seg->posA   = (vec4s){r->points[idxA].x, r->points[idxA].y, r->points[idxA].z, r->width * tA};
        seg->posB   = (vec4s){r->points[idxB].x, r->points[idxB].y, r->points[idxB].z, r->width * tB};
        seg->colorA = (vec4s){r->color.x, r->color.y, r->color.z, r->color.w * tA};
        seg->colorB = (vec4s){r->color.x, r->color.y, r->color.z, r->color.w * tB};
        seg->uv     = (vec4s){startV * r->stretch, endV * r->stretch, r->uv.x, r->uv.y};
    }
}