#include "world.h"
#include "render/render.h"

const Buff buff_kinds[BUFFKIND_COUNT] = {
    [BUFFKIND_FIRE] =
        {
            .damage   = 2.0f,
            .rate     = 0.5f,
            .duration = 3.0f,
            .power    = 1.0f,
        },
};

const char *buff_names[BUFFKIND_COUNT] = {
    [BUFFKIND_FIRE] = "BUFFKIND_FIRE",
};

static inline void Fire_OnDraw(World *world, int id, Buff *buff)
{
    QuadSSBO *quad = Sol_Render_GetNextQuad(PIPE_QUAD);
    quad->color    = (vec4s){1, 0.2f, 0, 1};
    quad->pos = (vec4s){world->xform.draw_pos[id].x, world->xform.draw_pos[id].y, world->xform.draw_pos[id].z, 1.0f};
    quad->textureId = SOL_TEXTURE_FIREPARTICLE;
    quad->rect      = (vec4s){0, 0, 1, 1};
    quad->uv        = (vec4s){0, 0, 1, 1};
}
static inline void Fire_OnApply(World *world, int id, Buff *buff)
{
}
static inline void Fire_OnRemove(World *world, int id, Buff *buff)
{
}
static inline void Fire_OnUpdate(World *world, int id, Buff *buff)
{
    if (buff->accum >= buff->rate)
    {
        buff->accum -= buff->rate;
        ScCombat *combat = Sol_Comp_Get(world, id, ScCombat);
        if (combat)
        {
            vec3s pos = world->xform.pos[id];
            Sol_Combat_Hit(world, id,
                           (SolHit){
                               .kind   = HITKIND_FIRE,
                               .entA   = buff->source,
                               .entB   = id,
                               .damage = {.amount = buff->damage},
                               .power  = buff->power,
                               .pos    = pos,
                           });
        }
    }
}

typedef void (*On)(World *, int, Buff *);
static const struct
{
    On apply;
    On update;
    On remove;
    On draw;
} ons[BUFFKIND_COUNT] = {
    [BUFFKIND_FIRE] =
        {
            .apply  = Fire_OnApply,
            .remove = Fire_OnRemove,
            .update = Fire_OnUpdate,
            .draw   = Fire_OnDraw,
        },
};

void Buff_Update(World *world, double dt)
{
    float fdt = (float)dt;

    SparseSet_ScBuff *set = Sol_Comp_Set(world, ScBuff);
    int count             = set->cnt;
    for (int i = count; i-- > 0;)
    {
        int id        = set->dense[i];
        ScBuff *buffs = &set->data[i];
        int write     = 0;
        for (int b = 0; b < buffs->count; b++)
        {
            Buff *buff = &buffs->buffs[b];
            buff->elapsed += fdt;
            buff->accum += fdt;
            if (!buff->hasUpdated)
            {
                if (ons[buff->kind].apply)
                    ons[buff->kind].apply(world, id, buff);
                buff->hasUpdated = 1;
            }
            if (ons[buff->kind].update)
                ons[buff->kind].update(world, id, buff);
            if (!buff->inf && (buff->elapsed > buff->duration))
            {
                if (ons[buff->kind].remove)
                    ons[buff->kind].remove(world, id, buff);
                buffs->activeKindsMask &= ~BITC(buff->kind);
            }
            else
            {
                buffs->buffs[write++] = *buff;
                buffs->activeKindsMask |= BITC(buff->kind);
            }
        }
        buffs->count = write;
        if (buffs->count <= 0)
            Sol_Comp_Rem(world, id, ScBuff);
    }
}

void Buff_Draw(World *world, double dt)
{
    SparseSet_ScBuff *set = Sol_Comp_Set(world, ScBuff);
    for (int i = 0; i < set->cnt; i++)
    {
        int id        = set->dense[i];
        ScBuff buff_c = set->data[i];
        for (int b = 0; b < buff_c.count; b++)
        {
            Buff *buff = &buff_c.buffs[b];
            if (ons[buff->kind].draw)
                ons[buff->kind].draw(world, id, buff);
        }
    }
}

void Sol_Buff_Add(World *world, int id, BuffKind kind, u32 source, float power)
{
    ScBuff *buffs = Sol_Comp_Add(world, id, ScBuff);
    if (buffs->count < MAX_BUFFS)
    {
        Buff b                       = buff_kinds[kind];
        b.kind                       = kind;
        b.source                     = source;
        b.power                      = power;
        buffs->buffs[buffs->count++] = b;
    }
}

void Sol_Buff_AddE(World *world, int id, BuffKind kind, u32 source, float power, float duration)
{
    ScBuff *buffs = Sol_Comp_Add(world, id, ScBuff);
    if (buffs->count < MAX_BUFFS)
    {
        Buff b                       = buff_kinds[kind];
        b.kind                       = kind;
        b.source                     = source;
        b.duration                   = duration;
        b.power                      = power;
        buffs->buffs[buffs->count++] = b;
    }
}

Buff *Sol_Buff_Next(World *world, int id, BuffKind kind)
{
    ScBuff *buffs = Sol_Comp_Add(world, id, ScBuff);
    if (buffs->count < MAX_BUFFS)
    {
        Buff *b = &buffs->buffs[buffs->count++];
        *b      = buff_kinds[kind];
        b->kind = kind;
        return b;
    }
    return NULL;
}

void Sol_Buff_Rem(World *world, int id, BuffKind kind)
{
    ScBuff *buffs = Sol_Comp_Add(world, id, ScBuff);
    for (int i = 0; i < buffs->count; i++)
    {
        Buff *buff = &buffs->buffs[i];
        if (buff->kind == kind)
        {
            buff->inf     = 0;
            buff->elapsed = buff->duration;
        }
    }
}

void Sol_Buff_AddMask(World *world, int id, u32 mask, u32 source, float power)
{
    ScBuff *buffs = Sol_Comp_Add(world, id, ScBuff);
    for (int i = 0; i < BUFFKIND_COUNT; i++)
        if (mask & (1 << i))
            Sol_Buff_Add(world, id, (BuffKind)i, source, power);
}

bool Sol_Buff_HasBuff(World *world, int id, BuffKind kind)
{
    ScBuff *buff = Sol_Comp_Get(world, id, ScBuff);
    if (!buff)
        return false;
    return buff->activeKindsMask & BITC(kind);
}