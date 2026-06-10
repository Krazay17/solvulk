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

static void Ribbon_AppendPoint(Ribbon *r, vec3s pos);
static void Ribbon_Tick_One(Ribbon *r, float fdt);
static void Ribbon_Draw_One(Ribbon *r);
static void Ribbon_Tick(World *world, double dt, double time);
static void Ribbon_Step(World *world, double dt, double time);
static void Sol_Ribbon_Draw(World *world, double dt, double time);

// --- default templates, mirrors emitter_kinds[] ---
static const Ribbon ribbon_kinds[RIBBONKIND_COUNT] = {
    [RIBBONKIND_TRAIL] =
        {
            .segLifetime = 0.3f,
            .rate        = .1f,
            .width       = 1.5f,
            .fadeout     = 0.4f,
            .color       = {1, 1, 1, 1},
            .inf         = true,
            .ttl         = 2.0f,
        },
    [RIBBONKIND_LIGHTNING] =
        {
            .segLifetime = 0.08f,
            .rate        = 0.0f,
            .width       = 0.08f,
            .fadeout     = 0.6f,
            .color       = {0.6f, 0.8f, 1.0f, 1.0f},
            .inf         = false,
            .ttl         = 0.5f,
        },
};

void Sol_Ribbon_Init(World *world)
{
    WAddStep(world) = Ribbon_Step; // entity-attached path
    WAddTick(world) = Ribbon_Tick; // world-spawned pool path
    WAdd3d(world)   = Sol_Ribbon_Draw;

    world->ribbons     = malloc(sizeof(SolRibbons));
    world->compRibbons = calloc(MAX_ENTS, sizeof(CompRibbon));

    world->ribbons->ribbon_count    = 0;
    world->ribbons->ribbon_capacity = MAX_RIBBONS;
    world->ribbons->ribbon          = calloc(MAX_RIBBONS, sizeof(Ribbon));
}

void Sol_Ribbon_Add(World *world, int id, RibbonKind kind, float width, vec4s color)
{
    CompRibbon *cr = &world->compRibbons[id];
    if (!(world->masks[id] & HAS_RIBBON))
        cr->ribbonCount = 0;
    if (cr->ribbonCount >= MAX_COMPRIBBONS)
        return;

    Ribbon *r   = &cr->ribbons[cr->ribbonCount++];
    *r          = ribbon_kinds[kind];
    r->width    = width;
    r->followId = id;
    r->alive    = true;
    if (color.a > 0)
        r->color = color;

    world->masks[id] |= HAS_RIBBON;
}

void Sol_Ribbon_Spawn(World *world, RibbonKind kind, vec3s pos)
{
    SolRibbons *s = world->ribbons;
    Sol_Realloc(&s->ribbon, s->ribbon_count, &s->ribbon_capacity, sizeof(Ribbon));

    Ribbon *r = &s->ribbon[s->ribbon_count++];
    *r        = ribbon_kinds[kind];
    r->alive  = true;
    Ribbon_AppendPoint(r, pos);
}

// --- internal ---

static void Ribbon_AppendPoint(Ribbon *r, vec3s pos)
{
    r->head            = (r->head + 1) % MAX_RIBBON_SEGS;
    r->points[r->head] = pos;
    r->ages[r->head]   = 0.0f;
    if (r->count < MAX_RIBBON_SEGS)
        r->count++;
}

static void Ribbon_Tick_One(Ribbon *r, float fdt)
{
    // Age all segments; compact by trimming the tail
    int live = 0;
    for (int i = 0; i < r->count; i++)
    {
        int idx = ((r->head - i) % MAX_RIBBON_SEGS + MAX_RIBBON_SEGS) % MAX_RIBBON_SEGS;
        r->ages[idx] += fdt;
        if (r->ages[idx] < r->segLifetime)
            live++;
    }
    // Oldest segments are at the tail; trim count down to live
    // (they'll naturally be skipped in draw by age check)
    r->count = live < r->count ? live + 1 : r->count; // keep at least head
}

static void Ribbon_Tick(World *world, double dt, double time)
{
    if (!world->ribbons)
        return;

    float       fdt   = (float)dt;
    SolRibbons *s     = world->ribbons;
    int         write = 0;

    for (int i = 0; i < s->ribbon_count; i++)
    {
        Ribbon *r = &s->ribbon[i];

        if (!r->inf)
        {
            r->ttl -= fdt;
            if (r->ttl <= 0)
                continue;
        }

        if (r->followId)
        {
            vec3s pos = Sol_Xform_GetPos(world, r->followId);
            r->accumulator += /* dist to last point — fill in with vecDist */ 0.0f;
            if (r->rate == 0 || r->accumulator >= r->rate)
            {
                Ribbon_AppendPoint(r, pos);
                r->accumulator = 0;
            }
        }

        Ribbon_Tick_One(r, fdt);
        s->ribbon[write++] = *r;
    }
    s->ribbon_count = write;
}

static void Ribbon_Step(World *world, double dt, double time)
{
    float fdt = (float)dt;

    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if (!(world->masks[id] & HAS_RIBBON))
            continue;

        CompRibbon *cr    = &world->compRibbons[id];
        vec3s       pos   = Sol_Xform_GetPos(world, id);
        int         write = 0;

        for (int b = 0; b < cr->ribbonCount; b++)
        {
            Ribbon *r = &cr->ribbons[b];

            if (!r->inf)
            {
                r->ttl -= fdt;
                if (r->ttl <= 0)
                    continue;
            }

            r->accumulator += vecDist(pos, r->points[r->head]);
            if (r->rate == 0 || r->accumulator >= r->rate)
            {
                Ribbon_AppendPoint(r, pos);
                r->accumulator = 0;
            }

            Ribbon_Tick_One(r, fdt);
            cr->ribbons[write++] = *r;
        }
        cr->ribbonCount = write;
        if (write == 0)
            world->masks[id] &= ~HAS_RIBBON;
    }
}

static void Ribbon_Draw_One(Ribbon *r)
{
    if (r->count < 2)
        return;

    for (int i = 0; i < r->count - 1; i++)
    {
        int idxA = ((r->head - i) + MAX_RIBBON_SEGS) % MAX_RIBBON_SEGS;
        int idxB = ((r->head - i - 1) + MAX_RIBBON_SEGS) % MAX_RIBBON_SEGS;

        float tA = 1.0f - (r->ages[idxA] / r->segLifetime);
        float tB = 1.0f - (r->ages[idxB] / r->segLifetime);
        tA       = tA < 0 ? 0 : tA;
        tB       = tB < 0 ? 0 : tB;

        RibbonSegSSBO *seg = Sol_Render_GetNext_RibbonSeg();
        if (!seg)
            return;

        seg->posA   = (vec4s){r->points[idxA].x, r->points[idxA].y, r->points[idxA].z, r->width * tA};
        seg->posB   = (vec4s){r->points[idxB].x, r->points[idxB].y, r->points[idxB].z, r->width * tB};
        seg->colorA = (vec4s){r->color.x, r->color.y, r->color.z, r->color.w * tA};
        seg->colorB = (vec4s){r->color.x, r->color.y, r->color.z, r->color.w * tB};
    }
}

static void Sol_Ribbon_Draw(World *world, double dt, double time)
{
    SolRibbons *s = world->ribbons;
    for (int i = 0; i < s->ribbon_count; i++)
        Ribbon_Draw_One(&s->ribbon[i]);

    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if (!(world->masks[id] & HAS_RIBBON))
            continue;
        CompRibbon *cr = &world->compRibbons[id];
        for (int b = 0; b < cr->ribbonCount; b++)
            Ribbon_Draw_One(&cr->ribbons[b]);
    }
}