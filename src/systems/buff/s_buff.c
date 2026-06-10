/*
 * File: s_buff.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * Buffs!
 */
#include "sol_core.h"

const Buff buff_config[BUFFKIND_COUNT] = {
    [BUFFKIND_FIRE] =
        {
            .freq     = 0.2f,
            .duration = 2.0f,
        },
    [BUFFKIND_INVULN] =
        {
            .duration = 0.2f,
            .addKind  = BUFFADDKIND_POWER_DURATION,
        },
};

void Sol_Buff_Init(World *world)
{
    world->stepSystems[world->stepCount++] = Sol_Buff_Step;
    world->buffs                           = calloc(MAX_ENTS, sizeof(CompBuff));
}

void Sol_Buff_AddFromMask(World *world, int id, BuffMask mask, int sourceId, float power)
{
    if (mask == 0)
        return;

    // Iterate through all possible buff indices
    for (int i = 0; i < BUFFKIND_COUNT; i++)
    {
        // Check if the bit for this specific buff index is set
        if (mask & (1ULL << i))
        {
            // Cast the index back to your regular sequential BuffKind enum
            BuffKind kind = (BuffKind)i;
            Sol_Buff_Add(world, id, kind, sourceId, power);
        }
    }
}

void Sol_Buff_Add(World *world, int id, BuffKind kind, int sourceId, float power)
{
    CompBuff *buffs = &world->buffs[id];
    if (!(world->masks[id] & HAS_BUFF))
        memset(buffs, 0, sizeof(CompBuff));
    world->masks[id] |= HAS_BUFF;
    if (buffs->count >= MAX_BUFFS)
        return;

    Buff new_buff   = buff_config[kind];
    new_buff.source = sourceId;
    new_buff.kind   = kind;
    new_buff.power  = power;

    // check if we have buffs already
    for (int i = 0; i < buffs->count; i++)
    {
        Buff *b = &buffs->buffs[i];
        if (kind == b->kind)
        {
            switch (new_buff.addKind)
            {
            case BUFFADDKIND_SET_DURATION:
                memcpy(b, &new_buff, sizeof(Buff));
                break;
            case BUFFADDKIND_ADD_DURATION:
                b->duration += new_buff.duration;
                break;
            case BUFFADDKIND_INF:
                b->inf = 1;
                break;
            case BUFFADDKIND_POWER_DURATION:
                b->duration = power;
                break;
            case BUFFADDKIND_MULTIPLY:
                goto addNewBuff;
            default:
                b->duration = new_buff.duration;
            }
            return;
        }
    }
addNewBuff:

    buffs->buffs[buffs->count++] = new_buff;
    buffs->activeKindsMask |= BuffBit(kind);
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

        CompBuff *buff    = &world->buffs[id];
        int       write   = 0;
        int       newmask = 0;
        for (int j = 0; j < buff->count; j++)
        {
            Buff *b = &buff->buffs[j];
            b->accum += fdt;
            b->duration -= fdt;
            if (b->duration <= 0 && !b->inf)
                continue;

            switch (b->kind)
            {
            case BUFFKIND_FIRE:
                Sol_Movement_SetSpeedMod(world, id, 0.7f);
                float interval = b->freq > 0 ? b->freq : BASE_TICK_INTERVAL;
                if (b->accum > interval)
                {
                    b->accum -= interval;
                    Sol_Event_Add(world, (SolEvent){.kind        = EVENTKIND_HIT,
                                                    .as.hit.entA = b->source,
                                                    .as.hit.damage = 2,
                                                    .as.hit.pos  = Sol_Xform_GetPos(world, id),
                                                    .as.hit.entB = id});
                }
                break;
            }

            buff->buffs[write++] = *b;
            newmask |= BuffBit(b->kind);
        }
        buff->count           = write;
        buff->activeKindsMask = newmask;

        if (buff->count <= 0)
            world->masks[id] &= ~HAS_BUFF;
    }
}

bool Sol_Buff_HasBuff(World *world, int id, BuffKind kind)
{
    return world->masks[id] & HAS_BUFF && (world->buffs[id].activeKindsMask & BuffBit(kind)) != 0;
}