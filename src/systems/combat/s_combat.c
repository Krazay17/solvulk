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
        case EVENTKIND_HIT: {
            if (!Sol_Owner_GetHostile(world, e->as.hit.entA, e->as.hit.entB))
                break;
            if (Sol_Buff_HasBuff(world, e->as.hit.entB, BUFFKIND_INVULN))
                break;
            if (Sol_Vital_GetDead(world, e->as.hit.entB))
                break;
            vec3s pos = e->as.hit.pos;
            for (int b = 0; b < e->as.hit.buffcount; b++)
            {
                Sol_Buff_Add(world, e->as.hit.entB, e->as.hit.buffKind[b], &e->as.hit);
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

            if (e->as.hit.power)
                Sol_Physx_Impulse(world, e->as.hit.entB, vecSca(e->as.hit.dir, e->as.hit.power));

            Sol_Vital_Damage(world, e->as.hit.entB, &e->as.hit);

            if (world->masks[e->as.hit.entB] & HAS_AICONTROLLER)
                Sol_AiController_SetLastHit(world, e->as.hit.entB, e->as.hit.entA, e->as.hit.damage);
        }
        break;

        case EVENTKIND_DEATH: {
            if (Net_IsClient())
                break;
            Sol_Vital_Die(world, e->as.death.entB);

            // Tell Ai their target died
            // for (int d = 0; d < world->activeCount; d++)
            // {
            //     int id = world->activeEntities[d];
            //     if ((world->masks[id] & HAS_AICONTROLLER))
            //         Sol_AiController_TargetDied(world, id, e->as.death.entB);
            // }
        }
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