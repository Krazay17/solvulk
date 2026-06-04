#include "sol_core.h"

typedef struct
{
    float    knockback, knockbackDuration, power;
    u32      damage, healing, buffCount, fxCount;
    BuffKind buffKinds[8];
    FxKind   fxKinds[8];
} HitData;

static HitData hit_kinds[HITKIND_COUNT] = {
    [HITKIND_FIREBALL] =
        {
            .damage  = 20,
            .fxCount = 1,
            .fxKinds = {FXKIND_FIREBALL_HIT},
        },
    [HITKIND_FIREBALL_EXPLODE] =
        {
            .damage            = 20,
            .knockback         = 20.0f,
            .knockbackDuration = 0.25f,
            .buffCount         = 2,
            .buffKinds         = {BUFFKIND_FIRE, BUFFKIND_KNOCKBACK},
            .fxCount           = 1,
            .fxKinds           = {FXKIND_FIRE_APPLY},
        },
    [HITKIND_SHIELD_PULSE] =
        {
            .damage            = 20,
            .knockback         = 20.0f,
            .knockbackDuration = 0.3f,
            .buffCount         = 1,
            .buffKinds         = {BUFFKIND_KNOCKBACK},
        },
    [HITKIND_FIRE] =
        {
            .damage = 2,

        },
    [HITKIND_BULLET] =
        {
            .damage = 10,
        },
};

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
            HitData hitData = hit_kinds[e->as.hit.kind];
            hitData.power   = e->as.hit.power > 0 ? e->as.hit.power : 1.0f;
            u32 damage      = (u32)((float)hitData.damage * hitData.power);
            e->as.hit.entA  = Sol_Owner_GetOwner(world, e->as.hit.entA);
            bool canDamage  = Sol_Owner_GetHostile(world, e->as.hit.entA, e->as.hit.entB) &&
                              !Sol_Buff_HasBuff(world, e->as.hit.entB, BUFFKIND_INVULN) &&
                              !Sol_Vital_GetDead(world, e->as.hit.entB);

            if (hitData.damage && canDamage)
            {
                for (int b = 0; b < hitData.buffCount; b++)
                    Sol_Buff_Add(world, e->as.hit.entB, hitData.buffKinds[b], e->as.hit.entA, 0, e->as.hit.power);

                Sol_Vital_Damage(world, e->as.hit.entB, e->as.hit.entA, damage);
            }

            if (canDamage && world->masks[e->as.hit.entB] & HAS_AICONTROLLER)
                Sol_AiController_SetLastHit(world, e->as.hit.entB, e->as.hit.entA, damage);

            if (canDamage && hitData.knockback)
            {
                vec3s vel = vecSca(glms_vec3_normalize(e->as.hit.vel), hitData.knockback * hitData.power);
                if (world->masks[e->as.hit.entB] & HAS_MOVEMENT)
                    Sol_Movement_SetKnockback(world, e->as.hit.entB, vel, hitData.knockbackDuration);
                else
                    Sol_Physx_Impulse(world, e->as.hit.entB, vel);
            }

            for (int fx = 0; fx < hitData.fxCount; fx++)
            {
                Sol_Event_Add(world, (SolEvent){
                                         .kind        = EVENTKIND_FX,
                                         .as.fx.kind  = hitData.fxKinds[fx],
                                         .as.fx.entB  = e->as.hit.entB,
                                         .as.fx.pos   = e->as.hit.pos,
                                         .as.fx.scale = e->as.hit.power,
                                     });
            }
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
void Sol_Combat_ClearHits(World *world, int id)
{
    memset(world->combats[id].hitEnts, 0, sizeof(world->combats[id].hitEnts));
}