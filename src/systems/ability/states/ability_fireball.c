#include "world.h"
#include "estate.h"
#include "sol_core.h"
#include "sol_math.h"

#include "render/render.h"
#include "prefabs.h"

#define MIN_POWER 0.2f
#define MELEE_DIST 2.0f

static vec3s GetProjectilePos(World *world, int id, float power, int slot)
{
    vec3s lookdir = Sol_RotFromQuat(Xform_GetDraw(world, id).rot);
    vec3s pos     = Sol_Model_GetBoneXform(world, id, slot == 1 ? "hand.R" : "hand.L").pos;
    if (glms_vec3_norm2(pos) == 0.0f)
        pos = Sol_Body3_GetHead(world, id);
    // pos = vecAdd(pos, vecSca(lookdir, (power - (MIN_POWER + 0.2f))));
    // pos = vecAdd(pos, vecSca(WORLD_UP, (power - (MIN_POWER + 0.6f))));
    return pos;
}

static void Charge(World *world, int id, ScAbility *ability, ScCmd *cmd, float dt)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];
    data->elapsed += dt;
    vec3s pos = GetProjectilePos(world, id, data->power, ability->activeSlot);

    SolHit hit = {
        .entA   = id,
        .power  = data->power,
        .damage = data->conf.damage,
    };

    switch (data->stage)
    {
    case 0:
        data->power = Sol_Math_Lerp_Clamped(MIN_POWER, data->conf.maxpower, data->elapsed / data->conf.duration);

        if (!data->held)
            data->stage++;
        break;
    case 1:
        data->stage++;
        vec3s dir = vecNorm(vecSub(cmd->aimpos, pos));

        { // Spawn fireball
            int fireball                   = Sol_Prefab_Fireball(world, id, pos, dir, 30.0f, data->power);
            ScProjectile *projectile       = Sol_Comp_Get(world, fireball, ScProjectile);
            projectile->hit                = hit;
            projectile->aoe_hit            = hit;
            projectile->aoe_hit.effectMask = data->conf.effectMask;
            projectile->aoe_hit.buffMask   = data->conf.buffMask;

            projectile->power = data->power;
        }
        break;
    case 2:
        hit.power = 1.0f;
        hit.kind = HITKIND_MELEE_HIT;
        SolRayResult results[256];
        Sol_Combat_DamageCast(
            world, id, (SolRay){.start = pos, .dir = cmd->aimdir, .dist = MELEE_DIST, .radius = 0.25f, .ignoreEnt = id},
            hit, results, 256, data->hitgen);
        data->recoverRemaining += dt;
        if (data->recoverRemaining > data->conf.recover)
            Sol_Ability_SetState(world, id, 0, ability->activeSlot, true);
        break;
    }
}

static void Spell(World *world, int id, ScAbility *ability, ScCmd *cmd, float dt)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];
    vec3s pos              = GetProjectilePos(world, id, 1.0f, ability->activeSlot);
    vec3s dir              = vecNorm(vecSub(cmd->aimpos, pos));

    { // Spawn fireball
        int fireball             = Sol_Prefab_Fireball(world, id, pos, dir, 30.0f, 1.0f);
        ScProjectile *projectile = Sol_Comp_Get(world, fireball, ScProjectile);
        projectile->hit          = (SolHit){
            .entA   = id,
            .damage = data->conf.damage,
            .power  = data->power,
        };
        projectile->aoe_hit = (SolHit){
            .entA       = id,
            .effectMask = data->conf.effectMask,
            .buffMask   = data->conf.buffMask,
            .damage     = data->conf.damage,
            .power      = data->power,
        };
        projectile->power = data->power;
    }

    Sol_Ability_SetState(world, id, 0, ability->activeSlot, true);
}
static void Dash(World *world, int id, ScAbility *ability, ScCmd *cmd, float dt)
{
}

void Ability_Fireball_Update(World *world, int id, ScAbility *ability, ScCmd *cmd, float dt)
{
    switch (ability->activeSlot)
    {
    case 0:
    case 1:
        Charge(world, id, ability, cmd, dt);
        break;
    case 2:
    case 3:
    case 4:
    case 5:
        Spell(world, id, ability, cmd, dt);
        break;
    case 6:
        Dash(world, id, ability, cmd, dt);
        break;
    }
}

void Ability_Fireball_Enter(World *world, int id, ScAbility *ability, ScCmd *cmd)
{
    AbilityStateData *data  = &ability->stateData[ability->activeSlot];
    data->conf              = Sol_Ability_GetSlotConf(ability, ability->activeSlot);
    data->hitgen            = Sol_Hitgen_Start(world, id);
    data->cooldownRemaining = data->conf.cooldown;
}

void Ability_Fireball_Exit(World *world, int id, ScAbility *ability, ScCmd *cmd)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];
}

bool Ability_Fireball_CanExit(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 next)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];
    return !data->recoverRemaining;
}

bool Ability_Fireball_CanEnter(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 last, int slot)
{
    AbilityStateData *data = &ability->stateData[slot];
    return data->cooldownRemaining <= 0.0f;
}

void Ability_Fireball_Draw(World *world, int id, ScAbility *ability)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];

    vec3s pos = GetProjectilePos(world, id, data->power, ability->activeSlot);
    switch (data->stage)
    {
    case 0: {
        *Sol_Render_GetNextSphere(PIPE_FIREBALL) = (SphereSSBO){
            .color = VEC4_RED,
            .pos   = (vec4s){pos.x, pos.y, pos.z, data->power * 0.5f},
        };
    }
    break;
    case 1:
    case 2: {
        *Sol_Render_GetNextSphere(PIPE_PLASMA) = (SphereSSBO){
            .color = VEC4_RED,
            .pos   = (vec4s){pos.x, pos.y, pos.z, 0.25f},
        };
    }
    break;
    }
}

extern const AbilityStateFunc ability_fireball_state = {
    .update   = Ability_Fireball_Update,
    .enter    = Ability_Fireball_Enter,
    .exit     = Ability_Fireball_Exit,
    .canExit  = Ability_Fireball_CanExit,
    .canEnter = Ability_Fireball_CanEnter,
    .draw     = Ability_Fireball_Draw,
};
