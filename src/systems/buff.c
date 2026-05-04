#include "sol_core.h"

#define MAX_BUFFS 64

typedef struct
{
    BuffKind kind;
    float duration;
} Buff;
typedef struct CompBuff
{
    Buff   buffs[MAX_BUFFS];
    u32    count;
    SolHit lasthit;
} CompBuff;

void Sol_Buff_Init(World *world)
{
    world->buffs = calloc(MAX_ENTS, sizeof(CompBuff));
}

void Sol_Buff_Add(World *world, int id, BuffDesc desc)
{

    CompBuff *buff = &world->buffs[id];
    if (buff->count >= MAX_BUFFS)
        return;
    world->masks[id] |= HAS_BUFF;
    for (int i = 0; i < buff->count; i++)
    {
        Buff *b = &buff->buffs[i];
        if (desc.kind == b->kind)
        {
            b->duration += desc.duration;
            return;
        }
    }
    buff->buffs[buff->count++] = (Buff){.kind = desc.kind, .duration = desc.duration};
}

void Sol_Buff_Remove(World *world, int id, BuffKind kind)
{
    CompBuff *buff = &world->buffs[id];
    for (int i = 0; i < buff->count; i++)
    {
        Buff *b = &buff->buffs[i];
        if (b->kind == kind)
        {
            b->duration = 0;
            return;
        }
    }
}

void Sol_Buff_Step(World *world, double dt, double time)
{
    float fdt      = (float)dt;
    int   required = HAS_BUFF;
    int   count    = world->activeCount;
    for (int i = 0; i < count; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;

        CompBuff *buff  = &world->buffs[id];
        int       write = 0;
        for (int j = 0; j < buff->count; j++)
        {
            Buff *b = &buff->buffs[j];
            b->duration -= fdt;
            if (b->duration <= 0)
                continue;

            if (b->kind & BUFF_KNOCKBACK)
            {
                // Sol_Physx_SetVel(world, id, glms_vec3_scale(buff->lasthit.dir, buff->lasthit.power));
            }
            buff->buffs[write++] = *b;
        }
        buff->count = write;

        if (buff->count <= 0)
            world->masks[id] &= ~HAS_BUFF;
    }
}