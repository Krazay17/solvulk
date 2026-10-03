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

#define CHARGE_DURATION 0.48f

#define HITDELAY 0.25f
#define HITINTERVAL 0.05f
#define MELEE_RANGE 4.0f
#define DASH_SPEED 30.0f
#define MIN_POWER 0.5f

#define CAST_TIME 0.45f
#define SWING_TIME 0.5f
#define RECOVER_TIME 0.33f

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
    switch (data->stage)
    {
    case 0:
        data->accum += dt;
        if (data->accum >= CAST_TIME)
        {
            data->accum = 0;
            data->stage++;
        }
        break;
    case 1:
        if (data->elapsed >= SWING_TIME)
        {
            Sol_Ability_SetState(world, id, 0, slot, true);
        }
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
    if (data->stage > 0)
    {
        if (data->hitPause > 0)
            data->hitPause = fmaxf(0, data->hitPause - dt * 5.0f);
        else
            data->elapsed += dt;
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
    switch (data->stage)
    {
    case 0:
        if (!data->held)
        {
            data->stage++;
            ability->prio_slot = slot;
            vec3s pos = GetProjectilePos(world, id, data->power, slot);
            vec3s dir = vecNorm(vecSub(cmd->aimpos, pos));

            SolHit hit = {
                .entA   = id,
                .power  = data->power,
                .damage = data->conf.damage,
            };

            // { // Spawn fireball
            //     int fireball             = Sol_Prefab_PlasmaOrb(world, id, pos, dir, 20.0f, data->power);
            //     ScProjectile *projectile = Sol_Comp_Get(world, fireball, ScProjectile);
            //     projectile->hit          = hit;
            //     projectile->aoe_hit      = hit;
            //     projectile->power        = data->power;
            // }
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
    case 1: {
        if (data->elapsed > HITDELAY)
        {
            vec3s head = Sol_Body3_GetHead(world, id);
            float width = 0.5f;
            ScBody3 *body3 = Sol_Comp_Get(world, id, ScBody3);
            if(body3)
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
    }
    break;
    }

    if (data->elapsed >= CHARGE_DURATION)
    {
        Sol_Ability_SetState(world, id, 0, slot, 1);
        return;
    }
    if (data->stage > 0)
    {
        if (data->hitPause > 0)
            data->hitPause = fmaxf(0, data->hitPause - dt * 5.0f);
        else
            data->elapsed += dt;
    }
}

void Ability_Claw_Enter(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot)
{
    AbilityStateData *data = &ability->stateData[slot];
    data->conf             = Sol_Ability_GetSlotConf(ability, slot);
    data->accum            = HITINTERVAL;
    data->hitgen           = Sol_Hitgen_Start(world, id);

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
}

DefendResult Defend(World *world, int id, ScAbility *ability, int slot, SolHit *hit)
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