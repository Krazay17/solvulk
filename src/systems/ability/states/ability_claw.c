/*
 * File: ability_claw.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-04
 *
 */
#include "ability/s_ability.h"
#include "world.h"
#include "estate.h"
#include "sol_core.h"
#include "sol_math.h"
#include "move3/s_move3.h"

#include "prefabs.h"
#include "render/render.h"

#define MIN_POWER 0.5f
#define MELEE_RANGE 3.0f
#define HITINTERVAL 0.05f
#define DASH_SPEED 30.0f

static const float spell_stage_time[3] = {0.4f, 0.15f, 0.25f};
#define SPELL_STAGE_COUNT (sizeof(spell_stage_time) / sizeof(spell_stage_time[0]))

static const float dash_stage_time[3] = {0.3f, 0.5f, 0.4f};
#define DASH_STAGE_COUNT (sizeof(dash_stage_time) / sizeof(dash_stage_time[0]))

static const float charge_stage_time[4] = {0.0f, 0.2f, 0.15f, 0.2f};
#define CHARGE_STAGE_COUNT (sizeof(charge_stage_time) / sizeof(charge_stage_time[0]))

static vec3s GetProjectilePos(World *world, int id, float power, int slot)
{
    vec3s pos = Sol_Model_GetBoneXform(world, id, slot > 5 ? "hand.R" : "hand.L").pos;
    if (glms_vec3_norm2(pos) == 0.0f)
        pos = Sol_Body3_GetHead(world, id);
    pos = vecAdd(pos, vecSca(WORLD_UP, (power * 0.5f)));
    return pos;
}

static void Spell(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot, float dt)
{
    AbilityStateData *data = &ability->stateData[slot];
    StageProgress prog     = Progress_Stage(data, dt, spell_stage_time, SPELL_STAGE_COUNT);
    if (prog.finished)
    {
        Sol_Ability_SetState(world, id, 0, slot, 1);
        return;
    }

    switch (prog.stage)
    {
    case 1:
        ScBody3 *body3 = Sol_Comp_Get(world, id, ScBody3);
        if (prog.advanced)
        {
            ScWeapon *weapon = Sol_Comp_Get(world, ability->left_weapon, ScWeapon);
            if (weapon)
                weapon->update_trail = true;
        }
        if (body3)
        {
            body3->vel.y = 5.0f;
        }
        vec3s hand_pos  = Sol_Model_GetBoneXform(world, id, slot > 5 ? "hand.R.Weapon" : "hand.L.Weapon").pos;
        vec3s blade_pos = Sol_Weapon_DmgPos(world, ability->left_weapon);
        SolDistDir dd   = Sol_GetDistDir(blade_pos, hand_pos);
        int hits        = Sol_Combat_DamageCast(world, id,
                                                (SolRay){
                                                    .dir       = dd.dir,
                                                    .dist      = dd.dist,
                                                    .ignoreEnt = id,
                                                    .radius    = 0.5f,
                                                    .start     = hand_pos,
                                                },
                                                (SolHit){
                                                    .damage = data->conf.damage,
                                                    .power  = data->power,
                                                    .entA   = id,
                                                    .kind   = HITKIND_MELEE_HIT,
                                                },
                                                data->hitgen);
        if (hits > 0)
            HitPause(data);
        break;
    case 2:
        ScWeapon *weapon = Sol_Comp_Get(world, ability->left_weapon, ScWeapon);
        if (weapon)
            weapon->update_trail = false;
        break;
    }
}

static void Dash(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot, float dt)
{
    AbilityStateData *data = &ability->stateData[slot];
    StageProgress prog     = Progress_Stage(data, dt, dash_stage_time, DASH_STAGE_COUNT);
    if (prog.finished)
    {
        Sol_Ability_SetState(world, id, 0, slot, 1);
        return;
    }
    ScWeapon *weaponL = Sol_Comp_Get(world, ability->left_weapon, ScWeapon);
    ScWeapon *weaponR = Sol_Comp_Get(world, ability->right_weapon, ScWeapon);
    switch (prog.stage)
    {
    case 1:
        int weaponId = slot > 5 ? ability->right_weapon : ability->left_weapon;
        if (prog.advanced)
        {
            Sol_Event_Push(world, EVENTKIND_FX,
                           (SolEvent){.as.fx.kind = FXKIND_SWORDSWING, .as.fx.pos = Sol_Body3_GetHead(world, id)});
            if (weaponL)
                weaponL->update_trail = true;
            if (weaponR)
                weaponR->update_trail = true;
        }

        float duration_delta = data->accum / dash_stage_time[data->stage];
        vec3s dir            = cmd->lookdir;
        // dir.y                = dir.y > 0 ? 0 : dir.y;
        ScBody3 *body3 = Sol_Comp_Get(world, id, ScBody3);
        if (body3)
        {
            if (data->hitPause == 0)
                body3->vel = vecSca(dir, Sol_Math_Remap(duration_delta, 0, 1.0f, DASH_SPEED, 5.0f));
            else
                body3->vel = GLMS_VEC3_ZERO;
        }

        vec3s hand_posL  = Sol_Model_GetBoneXform(world, id, "hand.L.Weapon").pos;
        vec3s hand_posR  = Sol_Model_GetBoneXform(world, id, "hand.R.Weapon").pos;
        vec3s blade_posL = Sol_Weapon_BladeXform(world, ability->left_weapon).pos;
        vec3s blade_posR = Sol_Weapon_BladeXform(world, ability->right_weapon).pos;
        SolDistDir ddL   = Sol_GetDistDir(blade_posL, hand_posL);
        SolDistDir ddR   = Sol_GetDistDir(blade_posR, hand_posR);

        SolRay rayA = {
            .start     = hand_posL,
            .dir       = ddL.dir,
            .dist      = ddL.dist,
            .ignoreEnt = id,
            .radius    = 0.3f,
        };
        SolRay rayB = {
            .start     = hand_posR,
            .dir       = ddR.dir,
            .dist      = ddR.dist,
            .ignoreEnt = id,
            .radius    = 0.3f,
        };
        SolHit hit = {
            .entA   = id,
            .damage = data->conf.damage,
            .power  = 0.5f,
            .kind   = HITKIND_MELEE_HIT,
        };
        int hits;
        hits = Sol_Combat_DamageCast(world, id, rayA, hit, data->hitgen);
        if (hits > 0)
            HitPause(data);
        hits = Sol_Combat_DamageCast(world, id, rayB, hit, data->hitgen2);
        if (hits > 0)
            HitPause(data);
        break;
    case 2:
        if (weaponL)
            weaponL->update_trail = false;
        if (weaponR)
            weaponR->update_trail = false;
    }
}

static void Charge(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot, float dt)
{
    AbilityStateData *data = &ability->stateData[slot];
    StageProgress prog     = {0};
    if (data->stage > 0)
        prog = Progress_Stage(data, dt, charge_stage_time, CHARGE_STAGE_COUNT);
    if (prog.finished)
    {
        Sol_Ability_SetState(world, id, 0, slot, 1);
        return;
    }

    int weaponId = slot > 5 ? ability->right_weapon : ability->left_weapon;
    switch (prog.stage)
    {
    case 0:
        if (!data->held)
        {
            data->stage++;
            ability->prio_slot = slot;
            Sol_Event_Push(world, EVENTKIND_FX,
                           (SolEvent){.as.fx.kind = FXKIND_SWORDSWING, .as.fx.pos = Sol_Body3_GetHead(world, id)});

            forc(world, id, ScBody3)
            {
                vec3s boost = cmd->lookdir;
                boost.y     = minf(boost.y, 0.0f);
                c->vel      = Accel_BringTo(boost, c->vel, 8.0f);
                forc(world, id, ScMove3)
                {
                    c->speedMod    = 0.5f;
                    c->frictionMod = 0.0f;
                }
            }
            break;
        }

        const float amount = dt * 40.0f;
        if (Sol_Combat_UseEnergy(world, id, amount))
            data->power += amount * 0.01f;
            
        break;
    case 2: {
        if (prog.advanced)
            forc(world, weaponId, ScWeapon) c->update_trail = true;

        vec3s hand_pos  = Sol_Model_GetBoneXform(world, id, slot > 5 ? "hand.R.Weapon" : "hand.L.Weapon").pos;
        vec3s blade_pos = Sol_Weapon_BladeXform(world, weaponId).pos;
        SolDistDir dd   = Sol_GetDistDir(blade_pos, hand_pos);
        SolRay ray      = {
            .start     = hand_pos,
            .dir       = dd.dir,
            .dist      = dd.dist * (1.0f + data->power),
            .ignoreEnt = id,
            .mask      = COLLAYER_ALL,
            .radius    = 0.33f,
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
            forc(world, id, ScBody3) c->vel.y = fmaxf(c->vel.y, 2.0f);
        }
    }
    break;
    case 3:
        forc(world, weaponId, ScWeapon) c->update_trail = false;
        break;
    }
}

void Ability_Claw_Enter(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot)
{
    AbilityStateData *data = &ability->stateData[slot];
    data->conf             = Sol_Ability_GetSlotConf(ability, slot);
    data->accum            = HITINTERVAL;
    data->hitgen           = Sol_Hitgen_Start(world);
    data->hitgen2          = Sol_Hitgen_Start(world);

    switch (slot_kind_map[slot])
    {
    case 0:
        data->power = data->conf.maxpower;
        break;
    case 1:
        data->power = MIN_POWER;
        break;
    }
}

void Spell_Enter(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot)
{
    Sol_Event_Push(world, EVENTKIND_FX,
                   (SolEvent){.as.fx.kind = FXKIND_SWORDSWING, .as.fx.pos = Sol_Body3_GetHead(world, id)});
    Ability_Claw_Enter(world, id, ability, cmd, slot);
}

void Ability_Claw_Exit(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot)
{
    AbilityStateData *data  = &ability->stateData[slot];
    data->cooldownRemaining = data->conf.cooldown;
    WeaponTrails_Off(world, ability);
}

bool Ability_Claw_CanExit(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot)
{
    AbilityStateData *data = &ability->stateData[slot];
    return true;
}

bool Ability_Claw_CanEnter(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot)
{
    AbilityStateData *data = &ability->stateData[slot];

    return !(data->cooldownRemaining > 0.0f);
}

void Draw_Charge(World *world, int id, ScAbility *ability, int slot, float dt)
{
    AbilityStateData *data = &ability->stateData[slot];
    Xform hand_xform       = Sol_Model_GetBoneXform(world, id, slot > 5 ? "hand.R.Weapon" : "hand.L.Weapon");
    float scale            = data->power * 1.5f;
    vec4s pos              = (vec4s){hand_xform.pos.x, hand_xform.pos.y, hand_xform.pos.z, scale};
    ModelSSBO *s           = Sol_Render_GetNextModel(0, MODELKIND_WEAPONBLADE);
    s->color               = (vec4s){1, 1, 1, 1};
    s->position            = pos;
    s->scale               = (vec4s){scale, scale * 1.5f, scale, scale};
    s->rotation            = (vec4s){hand_xform.rot.x, hand_xform.rot.y, hand_xform.rot.z, hand_xform.rot.w};
}

void Draw_Spell(World *world, int id, ScAbility *ability, int slot, float dt)
{
    AbilityStateData *data = &ability->stateData[slot];
    // Xform hand_xform       = Sol_Model_GetBoneXform(world, id, "hand.L");
    // float scale            = data->power;
    // vec4s pos              = (vec4s){hand_xform.pos.x, hand_xform.pos.y, hand_xform.pos.z, scale};

    // ModelSSBO *s = Sol_Render_GetNextModel(0, MODELKIND_WEAPONBLADE);
    // s->color     = (vec4s){1, 1, 1, 1};
    // s->position  = pos;
    // s->scale     = (vec4s){scale, scale, scale, scale};
    // s->rotation  = (vec4s){hand_xform.rot.x, hand_xform.rot.y, hand_xform.rot.z, hand_xform.rot.w};
}

DefendResult Defend(World *world, int id, ScAbility *ability, SolHit *hit, int slot)
{
    ScCmd *cmd   = Sol_Comp_Get(world, id, ScCmd);
    int attacker = hit->entA;
    vec3s delta  = glms_vec3_sub(hit->pos, Sol_Body3_GetHead(world, id));

    float dot = glms_vec3_dot(delta, cmd->lookdir);
    if (Sol_Comp_Has(world, attacker, ScProjectile) && dot > 0)
    {
        Sol_Combat_Reflect(world, attacker, id, hit->pos);
        return DEFENDKIND_CONSUMED;
    }
    return DEFENDKIND_NONE;
}

const AbilityStateFunc ability_claw_state = {
    .update   = Spell,
    .enter    = Spell_Enter,
    .exit     = Ability_Claw_Exit,
    .canExit  = Ability_Claw_CanExit,
    .canEnter = Ability_Claw_CanEnter,
    .draw     = Draw_Spell,
};

const AbilityStateFunc ability_claw_charge_state = {
    .update   = Charge,
    .enter    = Ability_Claw_Enter,
    .exit     = Ability_Claw_Exit,
    .canExit  = Ability_Claw_CanExit,
    .canEnter = Ability_Claw_CanEnter,
    .draw     = Draw_Charge,
};

const AbilityStateFunc ability_claw_dash_state = {
    .update   = Dash,
    .enter    = Ability_Claw_Enter,
    .exit     = Ability_Claw_Exit,
    .canExit  = Ability_Claw_CanExit,
    .canEnter = Ability_Claw_CanEnter,
    .defend   = Defend,
};