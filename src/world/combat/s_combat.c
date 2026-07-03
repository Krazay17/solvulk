#include "s_combat.h"
#include "sol_core.h"
#include "sol_math.h"
#include "network.h"
#include "world.h"
#include "audio.h"
#include "xform/s_xform.h"
#include "vital/s_vital.h"
#include "event/s_event.h"
#include "buff/s_buff.h"
#include "ability/s_ability.h"
#include "controller/s_controller.h"
#include "movement/s_movement.h"
#include "physx/s_body.h"
#include "owner/s_owner.h"
#include "item/s_item.h"

static void Combat_Step(World *world, double dt, double time);

void Sol_Combat_Init(World *world)
{
    world->combats               = calloc(MAX_ENTS, sizeof(CompCombat));
    world->dmgNumbers            = calloc(1, sizeof(Dmgnumbers));
    world->dmgNumbers->dmgNumber = calloc(1, sizeof(Dmgnumber));
    WAddStep(world)              = Combat_Step;
    WAddStep(world)              = Dmgnumbers_Step;
    WAdd3d(world)                = Dmgnumbers_Draw;
}

void Sol_Combat_Add(World *world, int id)
{
    world->masks[id] |= BITC(HAS_COMBAT);
}

static int  step_required = BITC(HAS_VITAL);
static void Combat_Step(World *world, double dt, double time)
{
    if (!Net_IsClient())
    {
        for (int i = 0; i < world->activeCount; i++)
        {
            int id = world->activeEntities[i];
            if (!WHas(world, id, step_required))
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
            float damage     = e->as.hit.damage;
            u32   effectMask = e->as.hit.effectMask;
            e->as.hit.power  = e->as.hit.power ? e->as.hit.power : 1.0f;
            e->as.hit.entA   = Sol_Owner_GetOwner(world, e->as.hit.entA);

            bool canDamage  = world->masks[e->as.hit.entB] & BITC(HAS_VITAL) &&
                              Sol_Owner_GetHostile(world, e->as.hit.entA, e->as.hit.entB) &&
                              !Sol_Buff_HasBuff(world, e->as.hit.entB, BUFFKIND_INVULN);
            bool targetDead = Sol_Vital_GetDead(world, e->as.hit.entB);

            if (canDamage)
            {
                if (damage)
                {
                    Sol_Buff_AddFromMask(world, e->as.hit.entB, e->as.hit.entA, e->as.hit.buffMask);

                    if (Sol_Vital_Damage(world, e->as.hit.entB, e->as.hit.entA, damage))
                    {
                        Sol_Dmgnumbers_Spawn(world, e->as.hit.entB, max((u32)damage, 1), e->as.hit.pos);
                        if (e->as.hit.entA == 1)
                        {
                            Sol_Audio_Play(SOL_AUDIO_HIT, 0.1f, 0.05f, 128);
                        }
                    }

                    Sol_Event_Add(world, (SolEvent){
                                             .kind       = EVENTKIND_FX,
                                             .as.fx.kind = e->as.hit.damageFx,
                                             .as.fx.pos  = e->as.hit.pos,
                                             .as.fx.entA = e->as.fx.entA,
                                             .as.fx.entB = e->as.fx.entB,
                                         });
                }
                if (world->masks[e->as.hit.entB] & BITC(HAS_AICONTROLLER))
                    Sol_AiController_SetLastHit(world, e->as.hit.entB, e->as.hit.entA, damage);

                if (effectMask & EFFECTMASK_HEALONHIT && !targetDead)
                {
                    Sol_Vital_Heal(world, e->as.hit.entA, e->as.hit.entA, 2);
                }
                if (effectMask & EFFECTMASK_CHAINLIGHTNING)
                {
                    float damage = Sol_Math_Lerp(0.01f, 5.0f, e->as.hit.power);
                    Sol_Chainhit_Trigger(world, e->as.hit.entA, e->as.hit.entB, CHAINKIND_LIGHTNING, damage);
                }
                float knockback         = 0;
                float knockbackDuration = 0;
                if (effectMask & EFFECTMASK_KNOCKBACK)
                {
                    knockback         = 15.0f;
                    knockbackDuration = 0.2f;
                }
                else if (effectMask & EFFECTMASK_KNOCKBACK_STRONG)
                {
                    knockback         = 25.0f;
                    knockbackDuration = 0.2f;
                }
                if (knockback)
                {
                    vec3s vel = vecSca(glms_vec3_normalize(e->as.hit.vel), knockback * e->as.hit.power);
                    if (world->masks[e->as.hit.entB] & BITC(HAS_MOVEMENT))
                        Sol_Movement_SetKnockback(world, e->as.hit.entB, vel, knockbackDuration);
                    else
                        Sol_Physx_Impulse(world, e->as.hit.entB, vel);
                }
                float knockup         = 0;
                float knockupDuration = 0;
                if (effectMask & EFFECTMASK_KNOCKUP)
                {
                    knockup         = 10.0f;
                    knockupDuration = 0.1f;
                }
                if (knockup)
                {
                    vec3s vel = vecSca(glms_vec3_normalize(WORLD_UP), knockup * e->as.hit.power);
                    if (world->masks[e->as.hit.entB] & BITC(HAS_MOVEMENT))
                        Sol_Movement_SetKnockback(world, e->as.hit.entB, vel, knockupDuration);
                    else
                        Sol_Physx_Impulse(world, e->as.hit.entB, vel);
                }
            }
            if (e->as.hit.fxKind)
                Sol_Event_Add(world, (SolEvent){
                                         .kind        = EVENTKIND_FX,
                                         .as.fx.kind  = e->as.hit.fxKind,
                                         .as.fx.entB  = e->as.hit.entB,
                                         .as.fx.pos   = e->as.hit.pos,
                                         .as.fx.scale = e->as.hit.power,
                                     });

            if (e->as.hit.effectMask & EFFECTMASK_REFLECTPROJECTILE &&
                world->masks[e->as.hit.entB] & BITC(HAS_PROJECTILE))
            {
                Sol_Physx_SetRedirectVel(world, e->as.hit.entB, Sol_Controller_GetAimdir(world, e->as.hit.entA));
                Sol_Owner_Add(world, e->as.hit.entB, e->as.hit.entA);
                Sol_Audio_PlayAt(SOL_AUDIO_PARRY, e->as.hit.pos, 0.5f, 0, 12);
            }
        }
        break;

        case EVENTKIND_DEATH: {
            Sol_Ability_SetState(world, e->as.death.entB, ABILITY_STATE_IDLE, -1, true);
            Sol_Movement_ForceState(world, e->as.death.entB, MOVE_DEAD);

            CompVital *vital = &world->vitals[e->as.death.entB];
            vital->deathTime = solState.gameTime;
            world->masks[e->as.death.entB] &= ~BITC(HAS_BUFF);

            Sol_Event_Add(world, (SolEvent){.kind       = EVENTKIND_FX,
                                            .as.fx.kind = FXKIND_DEATH_BLOOD,
                                            .as.fx.pos  = Sol_Xform_GetPos(world, e->as.death.entB)});

            Sol_Item_Drop(world, e->as.death.entB);
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

void Sol_Combat_AddFlags(World *world, int id, u32 flags)
{
    world->combats[id].flags |= flags;
}
void Sol_Combat_RemoveFlags(World *world, int id, u32 flags)
{
    world->combats[id].flags &= ~flags;
}
void Sol_Combat_ClearFlags(World *world, int id, u32 flags)
{
    world->combats[id].flags = 0;
}
void Sol_Combat_ClearHits(World *world, int id)
{
    memset(world->combats[id].hitEnts, 0, sizeof(world->combats[id].hitEnts));
}
