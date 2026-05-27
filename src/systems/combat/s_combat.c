#include "sol_core.h"

static void Combat_Step(World *world, double dt, double time);

void Sol_Combat_Init(World *world)
{
    WAddStep(world) = Combat_Step;
    world->combats  = calloc(MAX_ENTS, sizeof(CompCombat));
}

static void Combat_Step(World *world, double dt, double time)
{
    for (int i = 0; i < world->events->count; i++)
    {
        SolEvent *e = &world->events->event[i];
        switch (e->kind)
        {
        case EVENTKIND_HIT:
            if (!Sol_Owner_GetHostile(world, e->as.hit.source, e->as.hit.target))
                break;
            if (Sol_Buff_HasBuff(world, e->as.hit.target, BUFFKIND_INVULN))
                break;
            vec3s pos = e->as.hit.pos;
            for (int i = 0; i < e->as.hit.buffcount; i++)
            {
                Sol_Buff_Add(world, e->as.hit.target, e->as.hit.buffs[i], &e->as.hit);
            }

            switch (e->as.hit.kind)
            {
            case HITKIND_FIRE:
                Sol_Event_Add(world, (SolEvent){
                                         .kind       = EVENTKIND_FX,
                                         .as.fx.kind = FXKIND_FIRE_APPLY,
                                         .as.fx.pos  = pos,
                                     });
                break;
            }
            if(e->as.hit.power)
            Sol_Physx_Impulse(world, e->as.hit.target, vecSca(e->as.hit.dir, e->as.hit.power));

            Sol_Vital_Damage(world, e->as.hit.target, e->as.hit);

            if (world->masks[e->as.hit.target] & HAS_AICONTROLLER)
                Sol_AiController_SetLastHit(world, e->as.hit.target, e->as.hit.source, e->as.hit.damage);
            break;
        case EVENTKIND_DEATH:
            Sol_AiController_TargetDied(world, e->as.death.attacker, e->sourceId);
            break;
        }
    }
}

bool Sol_Combat_IsReflecting(World *world, int id)
{
    return world->combats[id].flags & COMBATFLAG_REFLECTING;
}

void Sol_Combat_AddFlags(World *world, int id, CombatFlags flags)
{
    world->combats[id].flags |= flags;
}
void Sol_Combat_RemoveFlags(World *world, int id, CombatFlags flags)
{
    world->combats[id].flags &= ~flags;
}
void Sol_Combat_ClearFlags(World *world, int id, CombatFlags flags)
{
    world->combats[id].flags = 0;
}