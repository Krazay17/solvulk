/*
 * File: s_buff.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * Buffs!
 */
#include "sol_core.h"

const Buff buff_config[] = {
    [BUFFKIND_FIRE] =
        {
            .kind     = BUFFKIND_FIRE,
            .addKind  = BUFFADDKIND_SET_DURATION,
            .freq     = 0.2f,
            .duration = 2.0f,
        },
    [BUFFKIND_INVULN] =
        {
            .kind     = BUFFKIND_INVULN,
            .duration = 0.2f,
        },
    [BUFFKIND_KNOCKBACK] =
        {
            .kind     = BUFFKIND_KNOCKBACK,
            .duration = 0.2f,
        },
};

void Sol_Buff_Init(World *world)
{
    world->stepSystems[world->stepCount++] = Sol_Buff_Step;
    world->buffs                           = calloc(MAX_ENTS, sizeof(CompBuff));
}

void Sol_Buff_Add(World *world, int id, BuffKind kind, const SolHit *hit)
{
    if (Sol_Vital_GetDead(world, id))
        return;
    world->masks[id] |= HAS_BUFF;
    CompBuff *buffs = &world->buffs[id];
    if (buffs->count >= MAX_BUFFS)
        return;

    Buff new_buff = buff_config[kind];

    // check if we have buffs already
    for (int i = 0; i < buffs->count; i++)
    {
        Buff *b = &buffs->buffs[i];
        if (kind == b->kind)
        {
            switch (new_buff.addKind)
            {
            case BUFFADDKIND_SET_DURATION:
                b->duration = new_buff.duration;
                break;
            case BUFFADDKIND_ADD_DURATION:
                b->duration += new_buff.duration;
                break;
            case BUFFADDKIND_INF:
                b->inf = 1;
                break;
            case BUFFADDKIND_MULTIPLY:
                goto addNewBuff;
            }
            return;
        }
    }
addNewBuff:
    if (hit)
    {
        new_buff.source = hit->source;
        new_buff.dir    = glms_normalize(hit->dir);
        switch (kind)
        {
        case BUFFKIND_KNOCKBACK:
            new_buff.dir   = glms_normalize(hit->dir);
            new_buff.power = hit->power;
            break;
        }
    }
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
                Sol_Movement_SetSpeedMod(world, id, 0.8f);
                float interval = b->freq > 0 ? b->freq : BASE_TICK_INTERVAL;
                if (b->accum > interval)
                {
                    b->accum -= interval;
                    Sol_Vital_Damage(world, id, (SolHit){.damage = 2, .source = b->source});
                }
                break;
            case BUFFKIND_KNOCKBACK:
                Sol_Physx_SetVel(world, id, vecSca(b->dir, b->power));
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
    return (world->buffs[id].activeKindsMask & BuffBit(kind)) != 0;
}