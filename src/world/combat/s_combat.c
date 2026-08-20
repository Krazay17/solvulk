#include "si_combat.h"
#include "sol_core.h"
#include "sol_math.h"
#include "network.h"
#include "world.h"
#include "audio.h"
#include "xform/s_xform.h"
#include "event/s_event.h"
#include "buff/s_buff.h"
#include "ability/s_ability.h"
#include "controller/s_controller.h"
#include "movement/s_movement.h"
#include "physx/s_body.h"
#include "owner/s_owner.h"
#include "item/s_item.h"

static const CompCombat combat_config[] = {
    [COMBATKIND_PLAYER] =
        {
            .maxHealth   = 100,
            .health      = 100,
            .maxEnergy   = 100,
            .energy      = 100,
            .energyRegen = 10.0f,
            .maxMana     = 100,
            .mana        = 100,
            .doesRespawn = 1,
            .respawnTime = 2.0f,
        },
    [COMBATKIND_WIZARD] =
        {
            .maxHealth   = 100,
            .health      = 100,
            .maxEnergy   = 100,
            .energy      = 100,
            .energyRegen = 10.0f,
            .maxMana     = 100,
            .mana        = 100,
            .doesRespawn = 1,
            .respawnTime = 2.0f,
        },
};

static void OnDeath(World *world, int id)
{
    Sol_Ability_SetState(world, id, ABILITY_STATE_IDLE, -1, true);
    Sol_Movement_ForceState(world, id, MOVE_DEAD);

    CompCombat *combat = &world->combats[id];
    combat->deathTime  = solState.gameTime;
    world->masks[id] &= ~BITC(HAS_BUFF);

    Sol_Item_Drop(world, id);

    Sol_Event_Add(world, (SolEvent){
                             .kind       = EVENTKIND_FX,
                             .as.fx.kind = FXKIND_DEATH_BLOOD,
                             .as.fx.pos  = Sol_Xform_GetPos(world, id),
                         });
    world->bodies[id].group = PHYSXMASK(0, 1);
}

static void OnRespawn(World *world, int id)
{
    CompCombat *combat = &world->combats[id];
    combat->health     = combat->maxHealth;
    combat->energy     = combat->maxEnergy;
    combat->mana       = combat->maxMana;
    Sol_Xform_Teleport(world, id, combat->respawnPos);
    Sol_Movement_SetState(world, id, MOVE_IDLE);
    world->bodies[id].group = world->bodies[id].base_group;
}

static void Combat_Step(World *world, double dt, double time)
{
    static int required = BITC(HAS_ACTIVE) | BITC(HAS_COMBAT);
    if (!Net_IsClient())
    {
        for (int i = 0; i < world->activeCount; i++)
        {
            int id = world->activeEntities[i];
            if (!WHas(world, id, required))
                continue;
            CompCombat *combat = &world->combats[id];
            if (combat->health == 0 && combat->doesRespawn && time > combat->deathTime + combat->respawnTime)
                OnRespawn(world, id);
            combat->energy = fmaxf(fminf(combat->energyRegen * dt + combat->energy, combat->maxEnergy), 0);
        }
    }
}

void Sol_Combat_Init(World *world)
{
    world->combats    = calloc(MAX_ENTS, sizeof(CompCombat));
    world->dmgNumbers = calloc(1, sizeof(Dmgnumbers));
    world->hitGen     = calloc(1, sizeof(HitGen));

    world->dmgNumbers->dmgNumber = malloc(sizeof(Dmgnumber));
    world->dmgNumbers->count     = 0;
    world->dmgNumbers->cap       = 0;

    world->chainhit           = malloc(sizeof(ChainAttacks));
    world->chainhit->capacity = CHAIN_CAP;
    world->chainhit->count    = 0;
    world->chainhit->chains   = calloc(CHAIN_CAP, sizeof(Chain));

    WAddStep(world) = Combat_Step;
    WAddStep(world) = Chain_Step;
    WAddStep(world) = Dmgnumbers_Step;
    WAdd3d(world)   = Dmgnumbers_Draw;
}

void Sol_Combat_Add(World *world, int id, CombatKind kind)
{
    CompCombat combat     = combat_config[kind];
    combat.lastHitTime    = -FLT_MAX;
    combat.respawnPos     = Sol_Xform_GetPos(world, id);
    combat.leftWeaponEnt  = -1;
    combat.rightWeaponEnt = -1;
    world->combats[id]    = combat;
    world->masks[id] |= BITC(HAS_COMBAT);
}
CompCombat *Sol_Combat_Get(World *world, int id)
{
    return &world->combats[id];
}

float Sol_Combat_Damage(World *world, int id, int attacker, float damage)
{
    CompCombat *combat      = &world->combats[id];
    float       damageDealt = 0.0f;
    if (combat->health == 0)
        return 0;

    if (damage >= combat->health)
    {
        if (combat->health > 0)
        {
            if (!Net_IsClient())
                OnDeath(world, id);
            combat->deathTime = solState.gameTime;
        }
        damageDealt    = combat->health;
        combat->health = 0;
    }
    else
    {
        combat->health -= damage;
        combat->lastHitTime = solState.gameTime;
        damageDealt         = damage;
    }

    Sol_Event_Add(world, (SolEvent){
                             .kind       = EVENTKIND_FX,
                             .as.fx.entA = attacker,
                             .as.fx.entB = id,
                             .as.fx.pos  = Sol_Xform_GetPos(world, id),
                             .as.fx.kind = FXKIND_TAKEDAMAGE,
                         });

    return damageDealt;
}

// pass in amount as percent
float Sol_Combat_HealPercent(World *world, int id, int dealer, float amount)
{
    if (!(world->masks[id] & BITC(HAS_COMBAT)))
        return 0;
    CompCombat *combat      = &world->combats[id];
    float       amountDealt = 0.0f;
    if (combat->health == 0)
        return 0;
    float finalAmount = combat->maxHealth * amount;

    if (combat->health + finalAmount >= combat->maxHealth)
    {
        combat->health = combat->maxHealth;
        amountDealt    = combat->maxHealth - combat->health;
    }
    else
    {
        combat->health += finalAmount;
        amountDealt = finalAmount;
    }

    Sol_Event_Add(world, (SolEvent){
                             .kind       = EVENTKIND_FX,
                             .as.fx.entA = dealer,
                             .as.fx.entB = id,
                             .as.fx.pos  = Sol_Xform_GetPos(world, id),
                             .as.fx.kind = FXKIND_TAKEHEALING,
                         });

    return amountDealt;
}

float Sol_Combat_Heal(World *world, int id, SolHit hit)
{
    CompCombat *combat      = &world->combats[id];
    float       amountDealt = 0.0f;
    if (combat->health == 0)
        return 0;

    if (combat->health + hit.damage >= combat->maxHealth)
    {
        combat->health = combat->maxHealth;
        amountDealt    = combat->maxHealth - combat->health;
    }
    else
    {
        combat->health += hit.damage;
        amountDealt = hit.damage;
    }

    Sol_Buff_AddFromMask(world, id, hit.entA, hit.buffMask);

    Sol_Event_Add(world, (SolEvent){
                             .kind       = EVENTKIND_FX,
                             .as.fx.entA = hit.entA,
                             .as.fx.entB = id,
                             .as.fx.pos  = Sol_Xform_GetPos(world, id),
                             .as.fx.kind = FXKIND_TAKEHEALING,
                         });

    return amountDealt;
}

void Sol_Combat_ApplyHit(World *world, int id, SolHit hit)
{
    float damage     = hit.damage;
    u32   effectMask = hit.effectMask;
    u32   buffMask   = hit.buffMask;
    float power      = hit.power ? hit.power : 1.0f;
    int   attacker   = Sol_Owner_GetOwner(world, hit.entA);

    bool canDamage  = world->masks[id] & BITC(HAS_COMBAT) && Sol_Owner_GetHostile(world, attacker, id) &&
                      !Sol_Buff_HasBuff(world, id, BUFFKIND_INVULN);
    bool targetDead = Sol_Combat_GetDead(world, id);

    if (canDamage)
    {
        if (damage)
        {
            Sol_Buff_AddFromMask(world, id, attacker, buffMask);
            float damageDealt = Sol_Combat_Damage(world, id, attacker, damage);
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
        if (world->masks[id] & BITC(HAS_AI))
            Sol_Ai_SetLastHit(world, id, attacker, damage);

        if (effectMask & EFFECTMASK_HEALONHIT && !targetDead)
        {
            Sol_Combat_Heal(world, attacker, (SolHit){.damage = 5, .entA = attacker, .entB = attacker});
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
        Sol_Physx_SetRedirectVel(world, id, Sol_Controller_Get(world, attacker)->aimdir);
        Sol_Owner_Add(world, id, attacker);
        Sol_Event_Add(world, (SolEvent){
                                 .kind            = EVENTKIND_SOUND,
                                 .as.sound.kind   = SOL_AUDIO_PARRY,
                                 .as.sound.pos    = hit.pos,
                                 .as.sound.volume = 1.0f,
                             });
    }
}

bool Sol_Combat_IsReflecting(World *world, int id)
{
    return world->combats[id].flags & COMBATFLAG_REFLECTING;
}

bool Sol_Combat_GetDead(World *world, int id)
{
    return (world->masks[id] & BITC(HAS_COMBAT)) && (world->combats[id].health == 0);
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

u32 Sol_Combat_StartHitGen(World *world, int id)
{
    HitGen *hitGen = world->hitGen;
    hitGen->globalHitGen++;
    if (hitGen->globalHitGen == 0)
    {
        memset(hitGen->hitGenMatrix, 0, sizeof(hitGen->hitGenMatrix));
        hitGen->globalHitGen = 1;
    }
    return hitGen->globalHitGen;
}

bool Sol_Combat_TryHitGen(World *world, int id, int target, u32 sessionGen)
{
    if (world->hitGen->hitGenMatrix[id][target] == sessionGen)
        return false;

    world->hitGen->hitGenMatrix[id][target] = sessionGen;
    return true;
}

float Sol_Combat_GetHealth(World *world, int id)
{
    if (!(world->masks[id] & BITC(HAS_COMBAT)))
        return 0;
    return world->combats[id].health;
}
