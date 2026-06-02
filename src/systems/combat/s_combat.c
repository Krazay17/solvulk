#include "sol_core.h"

static void Combat_Step(World *world, double dt, double time);

void Sol_Combat_Init(World *world)
{
    WAddStep(world) = Combat_Step;
    world->combats  = calloc(MAX_ENTS, sizeof(CompCombat));
}

static void Combat_Step(World *world, double dt, double time)
{
    if (!Net_IsClient())
    {
        int required = HAS_VITAL;
        for (int i = 0; i < world->activeCount; i++)
        {
            int id = world->activeEntities[i];
            if ((world->masks[id] & required) != required)
                continue;
            CompVital *vital = &world->vitals[id];
            if (Sol_Vital_GetDead(world, id) && vital->doesRespawn && time > vital->deathTime + vital->respawnTime)
            {
                Sol_Event_Add(world, (SolEvent){.kind = EVENTKIND_RESPAWN, .as.respawn.ent = id});
            }
        }
    }

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
            Sol_Movement_SetState(world, e->as.death.entB, MOVE_DEAD);

            CompVital *vital = &world->vitals[e->as.death.entB];
            vital->deathTime = Sol_GetGameTime();

            if (world->masks[e->as.death.entB] & HAS_BUFF)
                memset(&world->buffs[e->as.death.entB], 0, sizeof(CompBuff));

            Sol_Event_Add(world, (SolEvent){.kind       = EVENTKIND_FX,
                                            .as.fx.kind = FXKIND_DEATH_BLOOD,
                                            .as.fx.pos  = Sol_Xform_GetPos(world, e->as.death.entB)});

            if (!vital->doesRespawn)
                Sol_Destroy_Ent(world, e->as.death.entB);
        }
        break;
        case EVENTKIND_RESPAWN: {
            CompVital *vital = &world->vitals[e->as.respawn.ent];
            vital->health    = vital->maxHealth;
            vital->energy    = vital->maxEnergy;
            vital->mana      = vital->maxMana;
            Sol_Xform_Teleport(world, e->as.respawn.ent, e->as.respawn.pos);
            Sol_Movement_SetState(world, e->as.respawn.ent, MOVE_IDLE);
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