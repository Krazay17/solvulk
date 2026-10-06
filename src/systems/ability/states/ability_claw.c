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

static const float spell_stage_time[3] = {0.4f, 0.2f, 0.2f};
#define SPELL_STAGE_COUNT (sizeof(spell_stage_time) / sizeof(spell_stage_time[0]))

static const float charge_stage_time[4] = {0.0f, 0.25f, 0.15f, 0.2f};
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
    data->elapsed += dt;
    if (data->hitPause > 0)
        data->hitPause = fmaxf(0, data->hitPause - dt * 8.0f);
    else
        data->accum += dt;
    while (data->stage < SPELL_STAGE_COUNT && data->accum >= spell_stage_time[data->stage])
    {
        data->accum -= spell_stage_time[data->stage];
        data->stage++;
        switch (data->stage)
        {
        case 1:
            ScRibbon *ribbon = Sol_Ribbon_AddKind(world, id, RIBBONKIND_LIGHTNING_WEAPON_TRAIL);
            if (ribbon)
            {
                ribbon->ribbon.flags = RIBBONFLAG_NOFACECAM;
                ribbon->rate         = 0.02f;
                Xform bone_xform =
                    Sol_Model_GetBoneXform(world, id, slot > 5 ? "hand.R.Weapon.001" : "hand.L.Weapon.001");
                float spin = Sol_QuatGetRoll(bone_xform.rot, WORLD_FWD, WORLD_UP);
                spin += glm_rad(90.0f);
                Sol_Ribbon_Addpoint(&ribbon->ribbon, bone_xform.pos, 0, spin);
            }
            break;
        }
    }
    if (data->stage >= SPELL_STAGE_COUNT)
    {
        Sol_Ability_SetState(world, id, 0, slot, 1);
        return;
    }

    switch (data->stage)
    {
    case 1:
        ScBody3 *body3 = Sol_Comp_Get(world, id, ScBody3);
        if (body3)
        {
            body3->vel.y = 5.0f;
        }
        int hits = Sol_Combat_DamageCast(
            world, id,
            (SolRay){
                .dir       = cmd->aimdir,
                .dist      = MELEE_RANGE,
                .ignoreEnt = id,
                .radius    = 0.5f,
                .start     = glms_vec3_add(Sol_Body3_GetHead(world, id), glms_vec3_scale(cmd->aimdir, body3->dims.x)),
            },
            (SolHit){
                .damage = data->conf.damage,
                .power  = data->power,
                .entA   = id,
                .kind   = HITKIND_MELEE_HIT,
            },
            data->hitgen);
        if (hits > 0)
        {
            if (data->hitPauseDr < 4)
            {
                data->hitPause = data->hitPauseDr > 0 ? 1.0f / data->hitPauseDr : 1.0f;
                data->hitPauseDr++;
            }
        }
        break;
    }
}

static void Dash(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot, float dt)
{
    AbilityStateData *data = &ability->stateData[slot];
    ScBody3 *body3         = Sol_Comp_Get(world, id, ScBody3);
    if (!body3)
        return;
    vec3s pos            = world->xform.pos[id];
    float duration_delta = data->elapsed / data->conf.duration;
    vec3s dir            = cmd->lookdir;
    // dir.y                = dir.y > 0 ? 0 : dir.y;
    body3->vel  = vecSca(dir, Sol_Math_MapRange(DASH_SPEED, 5.0f, 0, 1.0f, duration_delta));
    float speed = glms_vec3_norm(body3->vel) * dt;

    SolRay ray = {
        .start     = pos,
        .dir       = cmd->lookdir,
        .dist      = 1.0f + speed,
        .ignoreEnt = id,
        .radius    = body3->dims.y,
    };
    SolHit hit = {
        .entA   = id,
        .damage = data->conf.damage,
        .power  = 1.0f,
        .kind   = HITKIND_MELEE_HIT,
    };
    Sol_Combat_DamageCast(world, id, ray, hit, data->hitgen);

    data->elapsed += dt;
    if (data->elapsed >= data->conf.duration)
        Sol_Ability_SetState(world, id, 0, slot, true);
}

static void Charge(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot, float dt)
{
    AbilityStateData *data = &ability->stateData[slot];
    data->elapsed += dt;
    if (data->stage > 0)
    {
        if (data->hitPause > 0)
            data->hitPause = fmaxf(0, data->hitPause - dt * 8.0f);
        else
            data->accum += dt;

        while (data->stage < CHARGE_STAGE_COUNT && data->accum >= charge_stage_time[data->stage])
        {
            data->accum -= charge_stage_time[data->stage];
            data->stage++;
            switch (data->stage)
            {
            case 2:
                ScRibbon *ribbon = Sol_Ribbon_AddKind(world, id, RIBBONKIND_LIGHTNING_WEAPON_TRAIL);
                if (ribbon)
                {
                    ribbon->ribbon.flags = RIBBONFLAG_NOFACECAM;
                    ribbon->rate         = 0.02f;
                    Xform bone_xform =
                        Sol_Model_GetBoneXform(world, id, slot > 5 ? "hand.R.Weapon.001" : "hand.L.Weapon.001");
                    float spin = Sol_QuatGetRoll(bone_xform.rot, WORLD_FWD, WORLD_UP);
                    spin += glm_rad(90.0f);
                    Sol_Ribbon_Addpoint(&ribbon->ribbon, bone_xform.pos, 0, spin);
                }
                break;
            }
        }
        if (data->stage >= CHARGE_STAGE_COUNT)
        {
            Sol_Ability_SetState(world, id, 0, slot, 1);
            return;
        }
    }

    switch (data->stage)
    {
    case 0:
        if (!data->held)
        {
            data->stage++;
            ability->prio_slot = slot;
            vec3s pos          = GetProjectilePos(world, id, data->power, slot);
            vec3s dir          = vecNorm(vecSub(cmd->aimpos, pos));

            SolHit hit = {
                .entA   = id,
                .power  = data->power,
                .damage = data->conf.damage,
            };

            ScBody3 *body3 = Sol_Comp_Get(world, id, ScBody3);
            if (body3)
            {
                vec3s boost = cmd->lookdir; // glms_vec3_norm2(cmd->wishdir) > 0 ? cmd->wishdir :
                boost.y     = boost.y < 0 ? boost.y : 0;
                body3->vel  = Accel_BringTo(boost, body3->vel, 8.0f);
            }
        }
        data->power = min(data->conf.maxpower, data->power + (dt * data->conf.speed));
        break;
    case 2: {
        vec3s head     = Sol_Body3_GetHead(world, id);
        float width    = 0.5f;
        ScBody3 *body3 = Sol_Comp_Get(world, id, ScBody3);
        if (body3)
        {
            width = body3->dims.x;
        }
        SolRay ray = {
            .start     = head,
            .dir       = cmd->aimdir,
            .dist      = width + MELEE_RANGE * data->power,
            .ignoreEnt = id,
            .mask      = COLLAYER_ALL,
            .radius    = 0.5f,
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
            if (data->hitPauseDr < 4)
            {
                data->hitPause = data->hitPauseDr > 0 ? 1.0f / data->hitPauseDr : 1.0f;
                data->hitPauseDr++;
            }
            ScBody3 *body = Sol_Comp_Get(world, id, ScBody3);
            if (body)
                body->vel.y = fmaxf(body->vel.y, 1.0f);
        }
    }
    break;
    }
}

void Ability_Claw_Enter(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot)
{
    AbilityStateData *data = &ability->stateData[slot];
    data->conf             = Sol_Ability_GetSlotConf(ability, slot);
    data->accum            = HITINTERVAL;
    data->hitgen           = Sol_Hitgen_Start(world);

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

void Ability_Claw_Exit(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot)
{
    AbilityStateData *data  = &ability->stateData[slot];
    data->cooldownRemaining = data->conf.cooldown;
    // Sol_Comp_Rem(world, id, ScRibbon);
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
    Xform hand_xform       = Sol_Model_GetBoneXform(world, id, slot > 5 ? "hand.R" : "hand.L");
    float scale            = data->power * 1.5f;
    vec4s pos              = (vec4s){hand_xform.pos.x, hand_xform.pos.y, hand_xform.pos.z, scale};
    ModelSSBO *s           = Sol_Render_GetNextModel(0, MODELKIND_WEAPONBLADE);
    s->color               = (vec4s){1, 1, 1, 1};
    s->position            = pos;
    s->scale               = (vec4s){scale, scale * 1.5f, scale, scale};
    s->rotation            = (vec4s){hand_xform.rot.x, hand_xform.rot.y, hand_xform.rot.z, hand_xform.rot.w};
    switch (data->stage)
    {
    case 2:
        ScRibbon *ribbon = Sol_Comp_Get(world, id, ScRibbon);
        if (ribbon)
        {
            ribbon->_accum += dt;
            if (ribbon->_accum >= ribbon->rate)
            {
                ribbon->_accum -= ribbon->rate;
                Xform bone_xform =
                    Sol_Model_GetBoneXform(world, id, slot > 5 ? "hand.R.Weapon.001" : "hand.L.Weapon.001");
                float spin = Sol_QuatGetRoll(bone_xform.rot, WORLD_FWD, WORLD_UP);
                spin += glm_rad(90.0f);

                Sol_Ribbon_Addpoint(&ribbon->ribbon, bone_xform.pos, 0, spin);
            }
        }
        break;
    }
}

void Draw_Spell(World *world, int id, ScAbility *ability, int slot, float dt)
{
    AbilityStateData *data = &ability->stateData[slot];
    Xform hand_xform       = Sol_Model_GetBoneXform(world, id, "hand.L");
    float scale            = data->power;
    vec4s pos              = (vec4s){hand_xform.pos.x, hand_xform.pos.y, hand_xform.pos.z, scale};

    ModelSSBO *s = Sol_Render_GetNextModel(0, MODELKIND_WEAPONBLADE);
    s->color     = (vec4s){1, 1, 1, 1};
    s->position  = pos;
    s->scale     = (vec4s){scale, scale, scale, scale};
    s->rotation  = (vec4s){hand_xform.rot.x, hand_xform.rot.y, hand_xform.rot.z, hand_xform.rot.w};
    switch (data->stage)
    {
    case 1:
        ScRibbon *ribbon = Sol_Comp_Get(world, id, ScRibbon);
        if (ribbon)
        {
            ribbon->_accum += dt;
            if (ribbon->_accum >= ribbon->rate)
            {
                ribbon->_accum -= ribbon->rate;
                Xform bone_xform =
                    Sol_Model_GetBoneXform(world, id, slot > 5 ? "hand.R.Weapon.001" : "hand.L.Weapon.001");
                float spin = Sol_QuatGetRoll(bone_xform.rot, WORLD_FWD, WORLD_UP);
                spin += glm_rad(90.0f);

                Sol_Ribbon_Addpoint(&ribbon->ribbon, bone_xform.pos, 0, spin);
            }
        }
        break;
    }
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
    .enter    = Ability_Claw_Enter,
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