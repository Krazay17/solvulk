#include "world.h"
#include "ability/s_ability.h"
#include "estate.h"
#include "sol_math.h"

#define MIN_POWER 0.2f

static const float charge_stage_time[5] = {0.0f, 0.25f, 0.25f, 0.2f, 0.2f};
#define CHARGE_STAGE_COUNT (sizeof(charge_stage_time) / sizeof(charge_stage_time[0]))

static const float spell_stage_time[4] = {0.0f, 0.3f, 0.5f, 0.2f};
#define SPELL_STAGE_COUNT (sizeof(spell_stage_time) / sizeof(spell_stage_time[0]))

static const float dash_stage_time[3] = {0.45f, 0.75f, 0.5f};
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
        }
        data->power = min(data->conf.maxpower, data->power + (dt * data->conf.speed));
        break;
    case 2: {
        int weaponId = slot > 5 ? ability->right_weapon : ability->left_weapon;
        if (prog.advanced)
        {
            forc(world, weaponId, ScWeapon) c->update_trail = true;
        }
        forc(world, id, ScBody3) c->vel                 = vecSca(cmd->lookdir, 4.0f);
        vec3s hand_pos  = Sol_Model_GetBoneXform(world, id, slot > 5 ? "hand.R.Weapon" : "hand.L.Weapon").pos;
        vec3s blade_pos = Sol_Weapon_BladeXform(world, weaponId).pos;
        SolDistDir dd   = Sol_GetDistDir(blade_pos, hand_pos);
        SolRay ray      = {
            .start     = hand_pos,
            .dir       = dd.dir,
            .dist      = dd.dist * (1.0f + data->power),
            .ignoreEnt = id,
            .mask      = COLLAYER_ALL,
            .radius    = 0.3f,
        };
        SolHit hit = {
            .kind   = HITKIND_MELEE_HIT,
            .damage = data->conf.damage,
            .power  = data->power,
            .entA   = id,
            .vel    = cmd->aimdir,
        };
        int hits = Sol_Combat_DamageCast(world, id, ray, hit, data->hitgen);
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
            data->hitgen                    = Sol_Hitgen_Start(world);
            data->hitPauseDr = 0;
        }
        forc(world, id, ScBody3) c->vel = vecSca(cmd->lookdir, -4.0f);
        SolHit hit = {
            .kind   = HITKIND_MELEE_HIT,
            .damage = data->conf.damage,
            .power  = data->power,
            .entA   = id,
            .vel    = cmd->aimdir,
        };
        int hits = Sol_Combat_DamageCast(world, id, WeaponTrace(world, id, ability, slot > 5), hit, data->hitgen);
        if (hits)
            HitPause(data);
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

    switch (prog.stage)
    {
    case 1:
        vec3s pos         = world->xform.pos[id];
        float stage_delta = data->accum / dash_stage_time[1];
        forc(world, id, ScBody3)
        {
            vec3s tangent  = vecCross(cmd->lookdir, WORLD_UP);
            float angle    = Sol_Math_Lerp(-45.0f, -120.0f, stage_delta);
            vec3s jump_dir = glms_vec3_rotate(WORLD_UP, glm_rad(angle), tangent);
            float boost    = Sol_Math_Lerp_Clamped(25.0f, 3.0f, stage_delta);
            vec3s jump_vel = vecSca(jump_dir, boost);

            c->vel = jump_vel;
        }
        break;
    case 2:
        ScBody3 *body = Sol_Comp_Get(world, id, ScBody3);
        if (!body)
            break;
        forc(world, id, ScMove3)
        {
            if (c->state == MOVE_FALL)
                body->vel.y = -30.0f;
        }
        SolHit hit = {
            .entA   = id,
            .damage = data->conf.damage,
            .power  = data->power,
            .kind   = HITKIND_MELEE_HIT,
        };
        int hits;
        hits = Sol_Combat_DamageCast(world, id, WeaponTrace(world, id, ability, 0), hit, data->hitgen);
        if (hits > 0)
        {
            body->vel = GLMS_VEC3_ZERO;
            HitPause(data);
        }
        hits = Sol_Combat_DamageCast(world, id, WeaponTrace(world, id, ability, 1), hit, data->hitgen2);
        if (hits > 0)
        {
            body->vel = GLMS_VEC3_ZERO;
            HitPause(data);
        }
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
        data->power = 1.0f;
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
