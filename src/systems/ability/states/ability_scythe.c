#include "world.h"
#include "ability/s_ability.h"
#include "estate.h"
#include "sol_math.h"

#define MIN_POWER 0.2f

static const float charge_stage_time[5] = {0.0f, 0.25f, 0.2f, 0.2f, 0.2f};
#define CHARGE_STAGE_COUNT (sizeof(charge_stage_time) / sizeof(charge_stage_time[0]))

static const float spell_stage_time[4] = {0.0f, 0.3f, 0.5f, 0.2f};
#define SPELL_STAGE_COUNT (sizeof(spell_stage_time) / sizeof(spell_stage_time[0]))

static const float dash_stage_time[4] = {0.4f, 0.7f, 0.4f, 0.3f};
#define DASH_STAGE_COUNT (sizeof(dash_stage_time) / sizeof(dash_stage_time[0]))

static void Charge(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot, float dt)
{
    AbilityStateData *data = &ability->stateData[slot];
    StageProgress prog     = {0};
    if (data->stage > 0)
    {
        prog = Progress_Stage(data, dt, charge_stage_time, CHARGE_STAGE_COUNT);

        forc(world, id, ScMove3) c->speedMod = 0.4f;
    }
    if (prog.finished)
        Sol_Ability_SetState(world, id, 0, slot, true);
    switch (prog.stage)
    {
    case 0:
        if (!data->held)
        {
            data->stage++;
            ability->prio_slot = slot;
            break;
        }

        const float amount = dt * 40.0f;
        if (Sol_Combat_UseEnergy(world, id, amount))
            data->power += amount * 0.01f;

        break;
    case 2: {
        int weaponId = slot > 5 ? ability->right_weapon : ability->left_weapon;
        if (prog.advanced)
        {
            forc(world, weaponId, ScWeapon) c->update_trail = true;
        }
        forc(world, id, ScBody3) c->vel = vecSca(cmd->lookdir, 5.0f);
        SolHit hit                      = {
            .kind   = HITKIND_MELEE_HIT,
            .damage = data->conf.damage,
            .power  = data->power,
            .entA   = id,
            .vel    = cmd->aimdir,
        };
        hit.damage.buffMask |= (1u << BUFFKIND_STUN);
        int hits =
            Sol_Combat_DamageCast(world, id, WeaponTrace(world, id, ability, slot > 5, data->power), hit, data->hitgen);
        if (hits > 0)
        {
            HitPause(data);
            ScBody3 *body3 = Sol_Comp_Get(world, id, ScBody3);
            if (body3)
                body3->vel.y = fmaxf(body3->vel.y, 1.0f);
        }
    }
    break;
    case 3: {
        if (prog.advanced)
        {
            data->hitgen     = Sol_Hitgen_Start(world);
            data->hitPauseDr = 0;
        }
        forc(world, id, ScBody3) c->vel = vecSca(cmd->lookdir, -5.0f);
        SolHit hit                      = {
            .kind   = HITKIND_MELEE_HIT,
            .damage = data->conf.damage,
            .power  = data->power,
            .entA   = id,
            .vel    = cmd->aimdir,
        };
        hit.damage.effectMask |= EFFECTMASK_PULL;
        int hits =
            Sol_Combat_DamageCast(world, id, WeaponTrace(world, id, ability, slot > 5, data->power), hit, data->hitgen);
        if (hits)
        {
            HitPause(data);
        }
    }
    break;
    case 4:
        WeaponTrails_Off(world, ability);
    }
}
static void Spell(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot, float dt)
{
    AbilityStateData *data = &ability->stateData[slot];
    Progress_Stage(data, dt, spell_stage_time, SPELL_STAGE_COUNT);
}
static void Dash(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot, float dt)
{
    AbilityStateData *data = &ability->stateData[slot];
    StageProgress prog     = {0};
    prog                   = Progress_Stage(data, dt, dash_stage_time, DASH_STAGE_COUNT);
    if (prog.finished)
        Sol_Ability_SetState(world, id, 0, slot, 1);

    SolHit hit = {
        .entA   = id,
        .damage = data->conf.damage,
        .power  = data->power,
        .kind   = HITKIND_MELEE_HIT,
    };
    hit.damage.amount *= 0.5f;

    switch (prog.stage)
    {
    case 1: {

        if (prog.advanced)
        {
            forc(world, ability->left_weapon, ScWeapon) c->update_trail  = true;
            forc(world, ability->right_weapon, ScWeapon) c->update_trail = true;
        }

        ScBody3 *body = Sol_Comp_Get(world, id, ScBody3);
        if (!body)
            break;

        int hits;
        hits = Sol_Combat_DamageCast(world, id, WeaponTrace(world, id, ability, 0, data->power), hit, data->hitgen);
        if (hits > 0)
        {
            body->vel = GLMS_VEC3_ZERO;
            HitPause(data);
        }
        hits = Sol_Combat_DamageCast(world, id, WeaponTrace(world, id, ability, 1, data->power), hit, data->hitgen2);
        if (hits > 0)
        {
            body->vel = GLMS_VEC3_ZERO;
            HitPause(data);
        }

        forc(world, id, ScBody3)
        {
            float stage_delta = glm_clamp(data->accum / dash_stage_time[1], 0.0f, 1.0f);

            // Build a horizontal direction, ignoring look pitch.
            vec3s forward = cmd->lookdir;
            forward.y     = 0.0f;

            if (glms_vec3_norm2(forward) < FLOAT_EPSILON)
                forward = (vec3s){{0.0f, 0.0f, -1.0f}};
            else
                forward = glms_vec3_normalize(forward);

            // Guaranteed nonzero rotation axis.
            vec3s tangent = glms_vec3_cross(forward, WORLD_UP);
            tangent       = glms_vec3_normalize(tangent);

            float angle    = glm_rad(Sol_Math_Lerp(-45.0f, -120.0f, stage_delta));
            vec3s jump_dir = glms_vec3_rotate(WORLD_UP, angle, tangent);
            jump_dir       = glms_vec3_normalize(jump_dir);

            float boost = Sol_Math_Lerp_Clamped(25.0f, 3.0f, stage_delta);

            c->vel = glms_vec3_scale(jump_dir, boost);
        }
    }
    break;
    case 2: {
        if (prog.advanced)
        {
            forc(world, ability->left_weapon, ScWeapon) c->update_trail  = true;
            forc(world, ability->right_weapon, ScWeapon) c->update_trail = true;

            data->hitgen  = Sol_Hitgen_Start(world);
            data->hitgen2 = Sol_Hitgen_Start(world);
        }
        ScBody3 *body = Sol_Comp_Get(world, id, ScBody3);
        if (!body)
            break;
        forc(world, id, ScMove3)
        {
            if (c->state == MOVE_FALL)
                body->vel.y = -30.0f;
        }
        int hits;
        hits = Sol_Combat_DamageCast(world, id, WeaponTrace(world, id, ability, 0, data->power), hit, data->hitgen);
        if (hits > 0)
        {
            body->vel = GLMS_VEC3_ZERO;
            HitPause(data);
        }
        hits = Sol_Combat_DamageCast(world, id, WeaponTrace(world, id, ability, 1, data->power), hit, data->hitgen2);
        if (hits > 0)
        {
            body->vel = GLMS_VEC3_ZERO;
            HitPause(data);
        }
    }
    break;
    case 3:
        WeaponTrails_Off(world, ability);
        break;
    }
}
static void Enter(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot)
{
    AbilityStateData *data = &ability->stateData[slot];
    data->hitgen           = Sol_Hitgen_Start(world);
    data->hitgen2          = Sol_Hitgen_Start(world);
    data->conf             = Sol_Ability_GetSlotConf(ability, slot);
    if (slot >= 5)
        data->power = MIN_POWER;
    else
        data->power = data->conf.maxpower;
}
static void Exit(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot)
{
    AbilityStateData *data = &ability->stateData[slot];
    WeaponTrails_Off(world, ability);
}
static bool CanExit(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot)
{
    return true;
}
static bool CanEnter(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot)
{
    AbilityStateData *data = &ability->stateData[slot];
    return !(data->cooldownRemaining > 0.0f);
}
static void Draw(World *world, int id, ScAbility *ability, int slot, float dt)
{
}
static DefendResult Defend(World *world, int id, ScAbility *ability, SolHit *hit, int slot)
{
}

const AbilityStateFunc ability_scythe_charge_state = {Charge, Enter, Exit, CanExit, CanEnter, Draw, Defend};
const AbilityStateFunc ability_scythe_spell_state  = {Charge, Enter, Exit, CanExit, CanEnter, Draw, Defend};
const AbilityStateFunc ability_scythe_dash_state   = {Dash, Enter, Exit, CanExit, CanEnter, Draw, Defend};
