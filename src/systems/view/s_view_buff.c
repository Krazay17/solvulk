#include "sol_core.h"

#include "buff/buff_i.h"

static void Sol_View_Buff_Tick(World *world, double dt, double time);

void Sol_View_Buff(World *world)
{
    WAdd3d(world)   = Sol_View_Buff_Draw;
    WAddTick(world) = Sol_View_Buff_Tick;
}

static void Sol_View_Buff_Tick(World *world, double dt, double time)
{
    static float accum = 0;
    accum += (float)dt;
    if (accum < 0.1f)
        return;
    accum        = 0;
    int required = HAS_BUFF;
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;

        CompXform *x = &world->xforms[id];
        CompBuff  *b = &world->buffs[id];
        for (int i = 0; i < b->count; i++)
        {
            switch (b->buffs[i].kind)
            {
            case BUFFKIND_FIRE:
                Sol_Emitter_Add(world, (Emitter){.pos      = x->pos,
                                                 .burst    = 10,
                                                 .particle = (Particle){.kind     = PARTICLE_FIRE,
                                                                        .ttl      = 1.0f,
                                                                        .color    = {.85f, 0.07f, 0.05f, 0.7f},
                                                                        .scale    = 0.25f,
                                                                        .speed    = 1.0f,
                                                                        .scalein  = .2f,
                                                                        .scaleout = .2f}});
                break;
            }
        }
    }
}

void Sol_View_Buff_Draw(World *world, double dt, double time)
{
    int required = HAS_BUFF;
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;
        CompXform *xform = &world->xforms[id];
        CompBuff  *b     = &world->buffs[id];
        for (int i = 0; i < b->count; i++)
        {
            switch (b->buffs[i].kind)
            {
            case BUFFKIND_FIRE:
                break;
            case BUFFKIND_INVULN:
                break;
            }
        }
    }
}