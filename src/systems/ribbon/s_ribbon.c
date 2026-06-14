/*
 * File: s_ribbon.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-06-10
 */
#include "sol_core.h"
#include <string.h>

#define INIT_CAPACITY 2048
#define INVALID_INDEX 0xFFFFFFFF

typedef struct RibbonSlot
{
    u32 dense_idx;  // Maps directly to the item's index in the dense array
    u32 generation; // Incremented on death to instantly invalidate dead handles
} RibbonSlot;

typedef struct SolRibbon
{
    // 1. The Dense Packed Arrays (Perfect for Stream Processing)
    Ribbon *ribbons;
    u32     ribbon_count;
    u32     ribbon_capacity;

    // 2. The Lookup Indirection Arrays (Perfect for Safe O(1) Lookups)
    RibbonSlot *slots;
    u32        *free_list;
    u32         slot_count;    // High water mark of allocated slots
    u32         slot_capacity; // Total slots allocated in heap
    u32         free_count;    // Active length of recycled slots array
} SolRibbon;

typedef struct
{
    RibbonKind       kind;
    RibbonAttachMode attachMode;
    vec4s            color;
    vec3s            pos;
    vec3s            targetPos;
    u32              followId;
    u32              targetId;
    float            width; // Overwrite default if > 0
} RibbonDesc;

static RibbonHandle Ribbon_Make(World *world, RibbonDesc desc);
static void         Ribbon_Step(World *world, double dt, double time);
static void         Ribbon_Draw(World *world, double dt, double time);
static void         Ribbon_AppendPoint(Ribbon *r, vec3s pos);
static void         Ribbon_Update_One(Ribbon *r, float fdt);
static void         Ribbon_Update_Trail(World *world, Ribbon *r, double dt, double time);
static void         Ribbon_Update_Beam(World *world, Ribbon *r, double dt, double time);
static void         Ribbon_Draw_Lightning(World *world, Ribbon *r, double dt, double time);
static void         Ribbon_Draw_Laser(World *world, Ribbon *r, double dt, double time);

typedef void (*RibbonFunc)(World *, Ribbon *, double, double);

typedef struct
{
    float            ttl;
    float            width;
    float            segLifetime;
    float            rate;
    int              segments;
    float            stretch;
    vec4s            color;
    u8               inf;
    u32              textureId;
    RibbonAttachMode attachMode;
    RibbonFunc       step, draw;
} RibbonConfig;

static const RibbonConfig ribbon_config[RIBBONKIND_COUNT] = {
    [RIBBONKIND_BULLETTRAIL] =
        {
            .inf         = 1,
            .width       = 0.05f,
            .segLifetime = 0.15f,
            .rate        = 0.02f,
            .segments    = 16,
            .stretch     = 1.0f,
            .color       = {1, 0.8f, 0.4f, 1},
            .textureId   = SOL_TEXTURE_BEAM,
            .attachMode  = RIBBONATTACH_TRAIL,
            .step        = Ribbon_Update_Trail,
            .draw        = Ribbon_Draw_Laser,
        },
    [RIBBONKIND_LIGHTNING] =
        {
            .ttl         = 0.2f,
            .width       = 0.52f,
            .segLifetime = 9999.f,
            .segments    = 2,
            .stretch     = 1.0f,
            .color       = {0.6f, 0.8f, 1.0f, 1.0f},
            .attachMode  = RIBBONATTACH_ENT_TO_ENT,
            .textureId   = SOL_TEXTURE_LIGHTNING,
            .step        = Ribbon_Update_Beam,
            .draw        = Ribbon_Draw_Lightning,
        },
    [RIBBONKIND_LASER] =
        {
            .inf         = 1,
            .width       = 0.08f,
            .segLifetime = 9999.f,
            .segments    = 2,
            .stretch     = 4.0f,
            .color       = {1, 0.2f, 0.2f, 1},
            .attachMode  = RIBBONATTACH_ENT_TO_POS,
            .textureId   = SOL_TEXTURE_BEAM,

            .step = Ribbon_Update_Beam,
            .draw = Ribbon_Draw_Laser,
        },
    [RIBBONKIND_INFBEAM] =
        {
            .inf         = 1,
            .width       = 0.1f,
            .segLifetime = 9999.f,
            .segments    = 2,
            .stretch     = 2.0f,
            .color       = {0.4f, 1.0f, 0.8f, 1.0f},
            .textureId   = SOL_TEXTURE_BEAM,
            .attachMode  = RIBBONATTACH_ENT_TO_POS,
            .step        = Ribbon_Update_Beam,
            .draw        = Ribbon_Draw_Laser,
        },
};

// --- Initialization ---
void Sol_Ribbon_Init(World *world)
{
    SolRibbon *s = malloc(sizeof(SolRibbon));

    s->ribbon_count    = 0;
    s->ribbon_capacity = INIT_CAPACITY;
    s->ribbons         = calloc(INIT_CAPACITY, sizeof(Ribbon));

    s->slot_count    = 0;
    s->slot_capacity = INIT_CAPACITY;
    s->free_count    = 0;
    s->slots         = calloc(INIT_CAPACITY, sizeof(RibbonSlot));
    s->free_list     = malloc(INIT_CAPACITY * sizeof(u32));

    world->ribbon = s;

    WAddStep(world) = Ribbon_Step;
    WAdd3d(world)   = Ribbon_Draw;
}

// --- Gameplay Modifiers ---
void Sol_Ribbon_UpdateTargetPos(World *world, RibbonHandle handle, vec3s newTargetPos)
{
    Ribbon *r = Sol_Ribbon_Get(world, handle);
    if (r)
        r->targetPos = newTargetPos;
}

// --- Internal Slot Allocator ---
static u32 Slot_Alloc(SolRibbon *s)
{
    if (s->free_count > 0)
    {
        return s->free_list[--s->free_count];
    }

    if (s->slot_count >= s->slot_capacity)
    {
        u32 old_cap = s->slot_capacity;
        u32 new_cap = old_cap * 2;

        s->slots     = realloc(s->slots, new_cap * sizeof(RibbonSlot));
        s->free_list = realloc(s->free_list, new_cap * sizeof(u32));

        // Zero out the newly opened slots memory completely
        memset(&s->slots[old_cap], 0, (new_cap - old_cap) * sizeof(RibbonSlot));

        // Seed generations to start cleanly at 1
        for (u32 i = old_cap; i < new_cap; i++)
        {
            s->slots[i].generation = 1;
        }
        s->slot_capacity = new_cap;
    }

    return s->slot_count++;
}

// --- Internal Handle Destroyer ---
static void Slot_Free(SolRibbon *s, u32 slot_index)
{
    s->slots[slot_index].generation++; // Automatically kills all existing handles
    s->slots[slot_index].dense_idx = INVALID_INDEX;
    s->free_list[s->free_count++]  = slot_index;
}

// --- O(1) Handle Lookups ---
Ribbon *Sol_Ribbon_Get(World *world, RibbonHandle handle)
{
    SolRibbon *s = world->ribbon;
    if (!s || handle.index >= s->slot_count)
        return NULL;

    RibbonSlot *slot = &s->slots[handle.index];
    if (slot->generation != handle.generation)
        return NULL; // Stale handle / dead element

    return &s->ribbons[slot->dense_idx];
}

bool Sol_Ribbon_IsAlive(World *world, RibbonHandle handle)
{
    return Sol_Ribbon_Get(world, handle) != NULL;
}

void Sol_Ribbon_Kill(World *world, RibbonHandle handle)
{
    Ribbon *r = Sol_Ribbon_Get(world, handle);
    if (!r)
        return;
    r->inf = 0;
    r->ttl = 0; // Automatically collected during next Ribbon_Step pass
}

// --- Internal: raw allocation, returns direct pointer ---
static RibbonHandle Ribbon_Make(World *world, RibbonDesc desc)
{
    SolRibbon *s = world->ribbon;

    if (s->ribbon_count >= s->ribbon_capacity)
    {
        s->ribbon_capacity *= 2;
        s->ribbons = realloc(s->ribbons, s->ribbon_capacity * sizeof(Ribbon));
    }

    u32 slot_idx  = Slot_Alloc(s);
    u32 dense_idx = s->ribbon_count++;

    s->slots[slot_idx].dense_idx = dense_idx;

    const RibbonConfig *cfg = &ribbon_config[desc.kind];
    Ribbon             *r   = &s->ribbons[dense_idx];

    *r = (Ribbon){
        .handle      = (RibbonHandle){.index = slot_idx, .generation = s->slots[slot_idx].generation},
        .kind        = desc.kind,
        .attachMode  = desc.attachMode,
        .followId    = desc.followId,
        .targetId    = desc.targetId,
        .targetPos   = desc.targetPos,
        .width       = desc.width > 0.0f ? desc.width : cfg->width,
        .color       = desc.color.a > 0.0f ? desc.color : cfg->color,
        .textureId   = cfg->textureId,
        .inf         = cfg->inf,
        .ttl         = cfg->ttl,
        .segLifetime = cfg->segLifetime,
        .rate        = cfg->rate,
        .segments    = cfg->segments > 0 ? cfg->segments : MAX_RIBBON_SEGS,
        .stretch     = cfg->stretch,
    };

    // Pre-warm configurations depending on type to maintain unified call paths
    if (r->attachMode == RIBBONATTACH_ENT_TO_ENT || r->attachMode == RIBBONATTACH_ENT_TO_POS)
    {
        r->count = r->segments;
    }
    else if (r->followId)
    {
        Ribbon_AppendPoint(r, desc.pos);
    }
    else
    {
        Ribbon_AppendPoint(r, desc.pos);
    }

    return r->handle;
}

// --- Public API ---

RibbonHandle Sol_Ribbon_Spawn(World *world, RibbonKind kind, vec3s pos, vec3s posB, vec4s color)
{
    RibbonDesc desc = {
        .kind       = kind,
        .attachMode = ribbon_config[kind].attachMode,
        .pos        = pos,
        .targetPos  = posB,
        .color      = color,
    };
    return Ribbon_Make(world, desc);
}

RibbonHandle Sol_Ribbon_Add(World *world, int id, RibbonKind kind, float width, vec4s color)
{
    RibbonDesc desc = {
        .kind       = kind,
        .attachMode = RIBBONATTACH_TRAIL,
        .followId   = id,
        .pos        = Sol_Xform_GetPos(world, id),
        .width      = width,
        .color      = color,
    };
    return Ribbon_Make(world, desc);
}

RibbonHandle Sol_Ribbon_AddWithTarget(World *world, int id, RibbonKind kind, vec3s targetPos, float width, vec4s color)
{
    RibbonDesc desc = {
        .kind       = kind,
        .attachMode = RIBBONATTACH_ENT_TO_POS,
        .followId   = id,
        .pos        = Sol_Xform_GetPos(world, id),
        .targetPos  = targetPos,
        .targetId   = 0,
        .width      = width,
        .color      = color,
    };
    return Ribbon_Make(world, desc);
}

RibbonHandle Sol_Ribbon_AddBetweenEntities(World *world, int entA, int entB, RibbonKind kind, float width, vec4s color)
{
    RibbonDesc desc = {
        .kind       = kind,
        .attachMode = RIBBONATTACH_ENT_TO_ENT,
        .followId   = entA,
        .targetId   = entB,
        .width      = width,
        .color      = color,
    };
    return Ribbon_Make(world, desc);
}

// --- Core Stream Step Loops ---
static void Ribbon_Step(World *world, double dt, double time)
{
    SolRibbon *s = world->ribbon;
    if (!s || s->ribbon_count == 0)
        return;

    float fdt       = (float)dt;
    u32   write_idx = 0;

    // Fast linear stream pass over dense memory arrays
    for (u32 read_idx = 0; read_idx < s->ribbon_count; read_idx++)
    {
        Ribbon *r = &s->ribbons[read_idx];

        // Synced entity lifetime cleanup check
        if (r->followId && !(world->masks[r->followId] & HAS_ACTIVE))
        {
            r->followId = 0;
            r->targetId = 0;
            r->inf      = 0;
        }

        if (!r->inf)
        {
            r->ttl -= fdt;
            if (r->ttl <= 0)
            {
                // Instantly clean up slot identifiers
                Slot_Free(s, r->handle.index);
                continue;
            }
        }

        ribbon_config[r->kind].step(world, r, dt, time);

        // Compress backwards if items died earlier in the sweep
        if (read_idx != write_idx)
        {
            s->ribbons[write_idx] = *r;
        }

        u32 slot_index                 = s->ribbons[write_idx].handle.index;
        s->slots[slot_index].dense_idx = write_idx;

        write_idx++;
    }

    s->ribbon_count = write_idx;
}

static void Ribbon_Draw(World *world, double dt, double time)
{
    SolRibbon *s = world->ribbon;
    if (!s)
        return;

    // Blistering fast hardware iteration path. Zero checks, zero branch misses.
    for (u32 i = 0; i < s->ribbon_count; i++)
    {
        ribbon_config[s->ribbons[i].kind].draw(world, &s->ribbons[i], dt, time);
    }
}

// --- Mathematical Layout Update Mechanics ---

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
    int live = 0;
    for (int i = 0; i < r->count; i++)
    {
        int idx = ((r->head - i) % r->segments + r->segments) % r->segments;
        r->ages[idx] += fdt;
        if (r->ages[idx] < r->segLifetime)
            live++;
    }
    r->count = live < r->count ? live + 1 : r->count;
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

static void Ribbon_Update_Beam(World *world, Ribbon *r, double dt, double time)
{
    if (!r->followId)
        return;

    vec3s startPos = Sol_Xform_GetDrawXform(world, r->followId).pos;
    vec3s endPos   = r->targetPos;

    if (r->attachMode == RIBBONATTACH_ENT_TO_ENT && r->targetId > 0)
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

// --- Graphical Draw Systems ---

static void Ribbon_Draw_Lightning(World *world, Ribbon *r, double dt, double time)
{
    if (r->count < 2)
        return;

    r->uv.x        = fmodf(r->uv.x + (float)(r->uvv.x * dt), 1.0f);
    r->uv.y        = fmodf(r->uv.y + (float)(r->uvv.y * dt), 1.0f);
    float invCount = 1.0f / (float)(r->count - 1);

    for (int i = 0; i < r->count - 1; i++)
    {
        int   idxA = ((r->head - i) + r->segments) % r->segments;
        int   idxB = ((r->head - i - 1) + r->segments) % r->segments;
        float tA   = glm_clamp(1.0f - (r->ages[idxA] / r->segLifetime), 0.0f, 1.0f);
        float tB   = glm_clamp(1.0f - (r->ages[idxB] / r->segLifetime), 0.0f, 1.0f);

        RibbonSegSSBO *seg = Sol_Render_GetNext_RibbonSeg(1);
        if (!seg)
            return;

        seg->posA   = (vec4s){r->points[idxA].x, r->points[idxA].y, r->points[idxA].z, r->width * tA};
        seg->posB   = (vec4s){r->points[idxB].x, r->points[idxB].y, r->points[idxB].z, r->width * tB};
        seg->colorA = (vec4s){r->color.x, r->color.y, r->color.z, r->color.w * tA};
        seg->colorB = (vec4s){r->color.x, r->color.y, r->color.z, r->color.w * tB};
        seg->uv = (vec4s){(float)i * invCount * r->stretch, (float)(i + 1) * invCount * r->stretch, r->uv.x, r->uv.y};
        seg->textureId = r->textureId > 0 ? r->textureId : SOL_TEXTURE_BEAM;
    }
}

static void Ribbon_Draw_Laser(World *world, Ribbon *r, double dt, double time)
{
    if (r->count < 2)
        return;

    r->uv.x             = fmodf(r->uv.x + (float)(r->uvv.x * dt), 1.0f);
    r->uv.y             = fmodf(r->uv.y + (float)(r->uvv.y * dt), 1.0f);
    float totalDistance = 0.0f;

    for (int i = 0; i < r->count - 1; i++)
    {
        int   idxA = ((r->head - i) + r->segments) % r->segments;
        int   idxB = ((r->head - i - 1) + r->segments) % r->segments;
        float tA   = glm_clamp(1.0f - (r->ages[idxA] / r->segLifetime), 0.0f, 1.0f);
        float tB   = glm_clamp(1.0f - (r->ages[idxB] / r->segLifetime), 0.0f, 1.0f);

        RibbonSegSSBO *seg = Sol_Render_GetNext_RibbonSeg(0);
        if (!seg)
            return;

        float startV = totalDistance;
        totalDistance += glms_vec3_distance(r->points[idxA], r->points[idxB]);

        seg->posA      = (vec4s){r->points[idxA].x, r->points[idxA].y, r->points[idxA].z, r->width * tA};
        seg->posB      = (vec4s){r->points[idxB].x, r->points[idxB].y, r->points[idxB].z, r->width * tB};
        seg->colorA    = (vec4s){r->color.x, r->color.y, r->color.z, r->color.w * tA};
        seg->colorB    = (vec4s){r->color.x, r->color.y, r->color.z, r->color.w * tB};
        seg->uv        = (vec4s){startV * r->stretch, totalDistance * r->stretch, r->uv.x, r->uv.y};
        seg->textureId = r->textureId > 0 ? r->textureId : SOL_TEXTURE_BEAM;
    }
}