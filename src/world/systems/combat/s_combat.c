#include "world.h"

typedef struct HitGen
{
    u32 hitGenMatrix[MAX_ENTS][MAX_ENTS];
    u32 globalHitGen;
} HitGen;

typedef struct
{
    HitGen hitgen;
} SysCombat;

static void OnRespawn(World *world, int id, ScCombat *combat)
{
    combat->health = combat->healthMax;
    combat->energy = combat->energyMax;
    combat->mana   = combat->manaMax;
}

static void OnDeath(World *world, int id, ScCombat *combat)
{
}

void Combat_Init(World *world)
{
    SysCombat *sys                  = malloc(sizeof(SysCombat));
    world->systems[WORLDSYS_COMBAT] = sys;
    memset(sys->hitgen.hitGenMatrix, 0, sizeof(sys->hitgen.hitGenMatrix));
    sys->hitgen.globalHitGen = 1;
}

void Combat_Step(World *world, double dt)
{
    SparseSet_ScCombat *set = Sol_Comp_Set(world, ScCombat);
    for (int i = 0; i < set->cnt; i++)
    {
        int id           = set->dense[i];
        ScCombat *combat = &set->data[i];

        if (combat->health <= 0)
        {
            if (combat->respawnTime && (combat->deathTime + combat->respawnTime > world->tickTime))
                OnRespawn(world, id, combat);
            OnDeath(world, id, combat);
        }
    }
}

float Sol_Combat_Hit(World *world, int id, SolHit hit)
{
    if (!Sol_Comp_Has(world, id, ScCombat))
        return 0;
    ScCombat *combat = Sol_Comp_Get(world, id, ScCombat);

    if (hit.isHeal)
        Sol_Combat_Heal(world, id, combat, hit.damage);
    else
        Sol_Combat_Damage(world, id, combat, hit.damage);
}

float Sol_Combat_Damage(World *world, int id, ScCombat *combat, float amount)
{
    if (combat->health <= 0)
        return 0;

    float damage_done = amount;

    if (combat->health <= amount)
    {
        damage_done    = combat->health;
        combat->health = 0;
        return damage_done;
    }
    combat->health -= amount;

    return damage_done;
}

float Sol_Combat_Heal(World *world, int id, ScCombat *combat, float amount)
{
    if (combat->health <= 0)
        return 0;

    float healing_done = amount;

    if (combat->health + amount > combat->healthMax)
    {
        healing_done   = combat->healthMax - combat->health;
        combat->health = combat->healthMax;
        return healing_done;
    }
    combat->health += amount;

    return healing_done;
}

u32 Sol_Combat_StartHitGen(World *world, int id)
{
    SysCombat *sys = world->systems[WORLDSYS_COMBAT];

    HitGen *hitgen = &sys->hitgen;
    hitgen->globalHitGen++;
    if (hitgen->globalHitGen == 0)
    {
        memset(hitgen->hitGenMatrix, 0, sizeof(hitgen->hitGenMatrix));
        hitgen->globalHitGen = 1;
    }
    return hitgen->globalHitGen;
}

bool Sol_Combat_TryHitGen(World *world, int id, int target, u32 sessionGen)
{
    SysCombat *sys = world->systems[WORLDSYS_COMBAT];

    if (sys->hitgen.hitGenMatrix[id][target] == sessionGen)
        return false;

    sys->hitgen.hitGenMatrix[id][target] = sessionGen;
    return true;
}
