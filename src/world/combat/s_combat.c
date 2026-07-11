#include "s_combat.h"
#include "sol_core.h"
#include "sol_math.h"
#include "network.h"
#include "world.h"
#include "audio.h"
#include "xform/s_xform.h"
#include "vital/s_vital.h"
#include "ai/s_ai.h"
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
    // WAddStep(world)              = Combat_Step;
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
        }
    }
}

void Sol_Combat_ApplyHit(World *world, int id, SolHit hit)
{
    float damage     = hit.damage;
    u32   effectMask = hit.effectMask;
    u32   buffMask   = hit.buffMask;
    float power      = hit.power ? hit.power : 1.0f;
    int   attacker   = Sol_Owner_GetOwner(world, hit.entA);

    bool canDamage  = world->masks[id] & BITC(HAS_VITAL) && Sol_Owner_GetHostile(world, attacker, id) &&
                      !Sol_Buff_HasBuff(world, id, BUFFKIND_INVULN);
    bool targetDead = Sol_Vital_GetDead(world, id);

    if (canDamage)
    {
        if (damage)
        {
            Sol_Buff_AddFromMask(world, id, attacker, buffMask);
            float damageDealt = Sol_Vital_Damage(world, id, attacker, damage);
            if (damageDealt)
            {
                Sol_Dmgnumbers_Spawn(world, id, max((u32)damage, 1), hit.pos);
                Sol_Event_Add(world, (SolEvent){
                                         .kind                 = EVENTKIND_SCORE,
                                         .entA                 = attacker,
                                         .entB                 = id,
                                         .as.score.damageDealt = damageDealt,
                                     });
                if (attacker == 1)
                {
                    Sol_Audio_Play(SOL_AUDIO_HIT, 0.1f, 0.05f, 128);
                }
            }
        }
        if (world->masks[id] & BITC(HAS_AICONTROLLER))
            Sol_AiController_SetLastHit(world, id, attacker, damage);

        if (effectMask & EFFECTMASK_HEALONHIT && !targetDead)
        {
            Sol_Vital_Heal(world, attacker, attacker, 2);
        }
        if (effectMask & EFFECTMASK_CHAINLIGHTNING)
        {
            float damage = Sol_Math_Lerp(0.01f, 5.0f, power);
            Sol_Chainhit_Trigger(world, attacker, id, CHAINKIND_LIGHTNING, damage);
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
            vec3s vel = vecSca(glms_vec3_normalize(hit.vel), knockback * power);
            if (world->masks[id] & BITC(HAS_MOVEMENT))
                Sol_Movement_SetKnockback(world, id, vel, knockbackDuration);
            else
                Sol_Physx_Impulse(world, id, vel);
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
            vec3s vel = vecSca(glms_vec3_normalize(WORLD_UP), knockup * power);
            if (world->masks[id] & BITC(HAS_MOVEMENT))
                Sol_Movement_SetKnockback(world, id, vel, knockupDuration);
            else
                Sol_Physx_Impulse(world, id, vel);
        }
    }

    if (hit.effectMask & EFFECTMASK_REFLECTPROJECTILE && world->masks[id] & BITC(HAS_PROJECTILE))
    {
        Sol_Physx_SetRedirectVel(world, id, Sol_Controller_GetAimdir(world, attacker));
        Sol_Owner_Add(world, id, attacker);
        Sol_Audio_PlayAt(SOL_AUDIO_PARRY, hit.pos, 0.5f, 0, 12);
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
