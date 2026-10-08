#include "world.h"
#include "ability/s_ability.h"
#include "estate.h"
#include "sol_math.h"

static const float charge_stage_time[4] = {0.0f, 0.3f, 0.5f, 0.2f};
#define CHARGE_STAGE_COUNT (sizeof(charge_stage_time) / sizeof(charge_stage_time[0]))

static void Charge(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot, float dt)
{
    AbilityStateData *data = &ability->stateData[slot];
    StageProgress prog     = {0};
    if (data->stage > 0)
        prog = Progress_Stage(data, dt, charge_stage_time, CHARGE_STAGE_COUNT);
    if (prog.finished)
        Sol_Ability_SetState(world, id, 0, slot, true);
    switch (prog.stage)
    {
    case 0:
        if (!data->held)
        {
            data->stage++;
        }
        data->power = min(data->conf.maxpower, data->power + (dt * data->conf.speed));
        break;
    case 2:
        int weaponId = slot > 5 ? ability->right_weapon : ability->left_weapon;
        if (prog.advanced)
        {
            ScWeapon *weapon = Sol_Comp_Get(world, weaponId, ScWeapon);
            if (weapon)
            {
                weapon->update_trail = true;
            }
        }
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
        break;
    case 3:
        WeaponTrails_Off(world, ability);
    }
}
static void Enter(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot)
{
    AbilityStateData *data = &ability->stateData[slot];
    data->hitgen           = Sol_Hitgen_Start(world);
    data->conf             = Sol_Ability_GetSlotConf(ability, slot);

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
const AbilityStateFunc ability_scythe_dash_state   = {Charge, Enter, Exit, CanExit, CanEnter, Draw, Defend};
