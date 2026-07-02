/*
 * File: s_buff.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * Buffs!
 */
#include "s_buff.h"
#include "world.h"
#include "xform/s_xform.h"
#include "event/s_event.h"
#include "ability/s_ability.h"
#include "movement/s_movement.h"
#include "emitter/s_emitter.h"
#include "vital/s_vital.h"
#include "render/render.h"

#define BASE_TICK_INTERVAL 1.0f

typedef void (*BuffEvent)(World *, int, int);
typedef void (*BuffTick)(World *world, int id, double dt, double time, Buff *b);
typedef struct BuffDesc
{
    u8        inf, kind, add, harmful;
    float     duration, freq, power, speedMod;
    BuffEvent onApply, onRemove;
    BuffTick  step, draw;
} BuffDesc;

static void Buff_Step(World *world, double dt, double time);
static void Buff_Draw(World *world, double dt, double time);
static void Buff_Make(World *world, int id, int source, BuffDesc desc);
static void Apply_Stun(World *world, int id, int source);
static void Remove_Stun(World *world, int id, int source);
static void Fire_Step(World *world, int id, double dt, double time, Buff *b);

const BuffDesc buff_config[BUFFKIND_COUNT] = {
    [BUFFKIND_STUN] =
        {
            .duration = 6.0f,
            .onApply  = Apply_Stun,
            .onRemove = Remove_Stun,
            .harmful  = 1,
        },
    [BUFFKIND_FIRE] =
        {
            .freq     = 0.2f,
            .duration = 6.0f,
            .speedMod = 0.7f,
            .add      = BUFFADD_ADD,
            .step     = Fire_Step,
            .harmful  = 1,
        },
    [BUFFKIND_INVULN] =
        {
            .duration = 0.2f,
            .add      = BUFFADD_SET,
            .harmful  = 0,
        },
};

// Public

void Sol_Buff_Init(World *world)
{
    world->buffs    = calloc(MAX_ENTS, sizeof(CompBuff));
    WAddStep(world) = Buff_Step;
    // WAdd3d(world)   = Buff_Draw;
}

void Sol_Buff_AddFromMask(World *world, int id, int source, u32 mask)
{
    if (mask == 0)
        return;
    //  Iterate through all possible buff indices
    for (int i = 0; i < BUFFKIND_COUNT; i++)
    {
        // Check if the bit for this specific buff index is set
        if (mask & (1ULL << i))
        {
            Sol_Buff_Add(world, id, source, i);
        }
    }
}

void Sol_Buff_Add(World *world, int id, int source, BuffKind kind)
{
    BuffDesc desc = buff_config[kind];
    desc.kind     = kind;
    Buff_Make(world, id, source, desc);
}

void Sol_Buff_AddEx(World *world, int id, int source, BuffKind kind, float duration, float power)
{
    BuffDesc desc = buff_config[kind];
    desc.kind     = kind;
    desc.duration = duration;
    desc.power    = power;
    Buff_Make(world, id, source, desc);
}

void Sol_Buff_Remove(World *world, int id, BuffKind kind)
{
    CompBuff *buffs = &world->buffs[id];
    int       write = 0;

    for (int i = 0; i < buffs->count; i++)
    {
        Buff *b = &buffs->buffs[i];
        if (b->kind == kind)
        {
            if (buff_config[kind].onRemove)
            {
                buff_config[kind].onRemove(world, id, b->source);
            }
            // Skip copying this element to compact the array immediately
            continue;
        }
        buffs->buffs[write++] = *b;
    }
    buffs->count = write;
    buffs->activeKindsMask &= ~BITC(kind);

    if (buffs->count <= 0)
        world->masks[id] &= ~BITC(HAS_BUFF);
}

u32 Sol_Buff_GetMask(World *world, int id)
{
    return world->buffs[id].activeKindsMask;
}

bool Sol_Buff_HasBuff(World *world, int id, BuffKind kind)
{
    return world->masks[id] & BITC(HAS_BUFF) && (world->buffs[id].activeKindsMask & BITC(kind)) != 0;
}

// Private

static void Buff_Make(World *world, int id, int source, BuffDesc desc)
{
    CompBuff *buffs = &world->buffs[id];
    if (!(world->masks[id] & BITC(HAS_BUFF)))
    {
        buffs->count           = 0;
        buffs->activeKindsMask = 0;
    }
    if (Sol_Vital_GetDead(world, id))
        return;

    world->masks[id] |= BITC(HAS_BUFF);

    Buff new_buff = {
        .source   = source,
        .kind     = desc.kind,
        .duration = desc.duration,
        .freq     = desc.freq,
        .inf      = desc.inf,
        .power    = desc.power,
        .harmful  = desc.harmful,
    };

    if (desc.add != BUFFADD_MULTIPLY)
    {
        for (int i = 0; i < buffs->count; i++)
        {
            Buff *b = &buffs->buffs[i];
            if (desc.kind == b->kind)
            {
                switch (desc.add)
                {
                case BUFFADD_ADD:
                    b->duration += new_buff.duration;
                    break;
                default:
                    b->duration = new_buff.duration;
                    break;
                }
                b->source = source;
                return;
            }
        }
    }

    if (buffs->count >= MAX_BUFFS)
        return;

    Buff *b = &buffs->buffs[buffs->count++];
    *b      = new_buff;
    if (buff_config[desc.kind].onApply)
        buff_config[desc.kind].onApply(world, id, source);
    buffs->activeKindsMask |= BITC(desc.kind);
}

static void Apply_Stun(World *world, int id, int source)
{
    Sol_Ability_SetState(world, id, 0, 0, true);
    Sol_Movement_ForceState(world, id, MOVE_STUN);
}
static void Remove_Stun(World *world, int id, int source)
{
    Sol_Movement_ForceState(world, id, MOVE_IDLE);
}

static void Fire_Step(World *world, int id, double dt, double time, Buff *b)
{
    Sol_Movement_SetSpeedMod(world, id, buff_config[b->kind].speedMod);
    float interval = b->freq > 0 ? b->freq : BASE_TICK_INTERVAL;
    if (b->accum > interval)
    {
        b->accum -= interval;
        Sol_Event_Add(world, (SolEvent){.kind          = EVENTKIND_HIT,
                                        .as.hit.entA   = b->source,
                                        .as.hit.damage = 2,
                                        .as.hit.pos    = Sol_Xform_GetPos(world, id),
                                        .as.hit.entB   = id});
        Sol_Emitter_Add(world, id, EMITTERKIND_POP_FIRE, (vec4s){1, 0, 0, 1}, 1.0f);
    }
}

static int  step_required = BITC(HAS_BUFF);
static void Buff_Step(World *world, double dt, double time)
{
    float fdt = (float)dt;
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if (!WHas(world, id, step_required))
            continue;

        CompBuff *buff    = &world->buffs[id];
        int       write   = 0;
        int       newmask = 0;
        for (int j = 0; j < buff->count; j++)
        {
            Buff *b = &buff->buffs[j];
            b->accum += fdt;
            b->duration -= fdt;
            if (b->duration <= 0 && !b->inf)
            {
                if (buff_config[b->kind].onRemove)
                    buff_config[b->kind].onRemove(world, id, b->source);
                continue;
            }
            if (buff_config[b->kind].step)
                buff_config[b->kind].step(world, id, dt, time, b);

            buff->buffs[write++] = *b;
            newmask |= BITC(b->kind);
        }
        buff->count           = write;
        buff->activeKindsMask = newmask;

        if (buff->count <= 0)
            world->masks[id] &= ~BITC(HAS_BUFF);
    }
}

static void Draw_Fire(World *world, double dt, double time)
{
    QuadSSBO *ssbo = Sol_Render_GetNext_Quad(QUADKIND_SPRITE_FRONT);
}

static UpdateFunc buff_draw_funcs[BUFFKIND_COUNT] = {
    [BUFFKIND_FIRE] = Draw_Fire,
};
static void Buff_Draw(World *world, double dt, double time)
{
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if (!WHas(world, id, step_required))
            continue;

        CompBuff *buff = &world->buffs[id];
    }
}