/*
 * File: s_buff.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * Buffs!
 */
#include "sol_core.h"

#include "buff_i.h"

void Sol_Buff_Init(World *world)
{
    world->stepSystems[world->stepCount++] = Sol_Buff_Step;
    world->buffs                           = calloc(MAX_ENTS, sizeof(CompBuff));
}

void Sol_Buff_Clear(World *world, int id)
{
    memset(&world->buffs[id], 0, sizeof(CompBuff));
}

void Sol_Buff_Add(World *world, int id, BuffDesc desc, const SolHit *hit)
{
    CompBuff *buff = &world->buffs[id];
    if (buff->count >= MAX_BUFFS)
        return;
    world->masks[id] |= HAS_BUFF;
    u32 infinite = desc.addKind == BUFFADD_INF ? 1 : 0;
    // check if we have buff already
    for (int i = 0; i < buff->count; i++)
    {
        Buff *b = &buff->buffs[i];
        if (desc.kind == b->kind)
        {
            switch (desc.addKind)
            {
            case BUFFADD_SET_DURATION:
                b->duration = desc.duration;
                break;
            case BUFFADD_ADD_DURATION:
                b->duration += desc.duration;
                break;
            case BUFFADD_INF:
                b->inf = 1;
                break;
            case BUFFADD_MULTIPLY:
                goto addAnotherBuff;
            }
            return;
        }
    }
// fallthrough: add buff if dont have
addAnotherBuff:
    Buff new_buff = {.kind = desc.kind, .duration = desc.duration, .inf = infinite, .freq = desc.freq};
    if (hit)
    {
        switch (desc.kind)
        {
        case BUFFKIND_KNOCKBACK:
            new_buff.dir   = glms_normalize(hit->dir);
            new_buff.power = hit->power;
            break;
        case BUFFKIND_FIRE:
            new_buff.source = hit->source;
            break;
        }
    }
    buff->buffs[buff->count++] = new_buff;
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
            b->accum += fdt;
            b->duration -= fdt;
            if (b->duration <= 0 && !b->inf)
                continue;

            switch (b->kind)
            {
            case BUFFKIND_FIRE:
                Sol_Movement_SetSpeedMod(world, id, 0.3f);
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
            default:
                printf("no buff kind\n");
            }
            buff->buffs[write++] = *b;
        }
        buff->count = write;

        if (buff->count <= 0)
            world->masks[id] &= ~HAS_BUFF;
    }
}

bool Sol_Buff_HasBuff(World *world, int id, BuffKind kind)
{
    for (int i = 0; i < world->buffs[id].count; i++)
    {
        if (world->buffs[id].buffs[i].kind == kind)
            return true;
    }
    return false;
}