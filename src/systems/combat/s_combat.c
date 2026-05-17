#include "sol_core.h"

#include "combat_i.h"

static void Combat_Step(World *world, double dt, double time);

void Sol_Combat_Init(World *world)
{
    WAddStep(world) = Combat_Step;
}

static void Combat_Step(World *world, double dt, double time)
{
    for (int i = 0; i < world->events->count; i++)
    {
        SolEvent *e = &world->events->event[i];
        if (e->kind != EVENT_HIT)
            continue;
        vec3s pos = e->as.hit.pos;
        for (int i = 0; i < e->as.hit.buffcount; i++)
        {
            Sol_Buff_Add(world, e->as.hit.target, e->as.hit.buffs[i], &e->as.hit);
        }

        switch (e->as.hit.kind)
        {
        case DAMAGEKIND_FIRE:
            Sol_Event_Add(world, (SolEvent){
                                     .kind        = EVENT_FX,
                                     .as.fx.kind  = FXKIND_FIRE_APPLY,
                                     .as.fx.pos   = pos,
                                 });
            break;
        }

        Sol_Vital_Damage(world, e->as.hit.target, e->as.hit.damage);
    }
}