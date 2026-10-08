/*
 * File: ability_fireball.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-28
 *
 */
#include "ability/s_ability.h"
#include "world.h"
#include "estate.h"
#include "sol_core.h"
#include "sol_math.h"

#include "render/render.h"
#include "prefabs.h"

#define MIN_POWER 0.2f
#define MELEE_DIST 4.5f
#define HIT_RATE 0.05f
#define HIT_DELAY 0.1f

#define CAST_TIME 0.7f
#define RECOVER_TIME 0.5f

static const float spell_stage_time[3] = {0.5f, 0.3f, 0.3f};
#define SPELL_STAGE_COUNT (sizeof(spell_stage_time) / sizeof(spell_stage_time[0]))

static const float charge_stage_time[4] = {0.0f, 0.2f, 0.15f, 0.2f};
#define CHARGE_STAGE_COUNT (sizeof(charge_stage_time) / sizeof(charge_stage_time[0]))

static vec3s GetProjectilePos(World *world, int id, float power, int slot)
{
    vec3s pos = Sol_Model_GetBoneXform(world, id, slot > 5 ? "hand.R.Weapon" : "hand.L.Weapon").pos;
    if (glms_vec3_norm2(pos) == 0.0f)
        pos = Sol_Body3_GetHead(world, id);
    pos = vecAdd(pos, vecSca(WORLD_UP, (power * 0.5f)));
    return pos;
}

static void Dash(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot, float dt)
{
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
            case 1:
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
            vec3s pos = GetProjectilePos(world, id, data->power, slot);
            vec3s dir = vecNorm(vecSub(cmd->aimpos, pos));

            SolHit hit = {
                .entA   = id,
                .power  = data->power,
                .damage = data->conf.damage,
            };
            hit.damage.amount *= 0.5f;
            { // Spawn fireball
                int fireball             = Sol_Prefab_Fireball(world, id, pos, dir, 25.0f, data->power);
                ScBody3 *pBody           = Sol_Comp_Get(world, fireball, ScBody3);
                ScProjectile *projectile = Sol_Comp_Get(world, fireball, ScProjectile);
                projectile->hit          = hit;
                projectile->aoe_hit      = hit;
                projectile->power        = data->power;
            }

            break;
        }
        data->power    = min(data->conf.maxpower, data->power + (dt * data->conf.speed));
        ScMove3 *move3 = Sol_Comp_Get(world, id, ScMove3);
        if (move3)
            move3->speedMod = Sol_Math_Lerp(1.0f, 0.5f, data->power / data->conf.maxpower);
        break;
    case 2:
        data->hitaccum += dt;
        if (data->hitaccum >= HIT_RATE)
        {
            data->hitaccum -= HIT_RATE;
            vec3s pos = GetProjectilePos(world, id, data->power, slot);
            vec3s dir = vecNorm(vecSub(cmd->aimpos, pos));

            SolHit hit = {
                .entA   = id,
                .kind   = HITKIND_MELEE_HIT,
                .power  = data->power,
                .damage = data->conf.damage,
                .power  = 0.1f,
            };
            Sol_Combat_DamageCast(world, id,
                                  (SolRay){.start     = pos,
                                           .dir       = dir,
                                           .dist      = MELEE_DIST,
                                           .radius    = 0.4f,
                                           .ignoreEnt = id,
                                           .mask      = COLLAYER_ALL},
                                  hit, 0);
        }
    }
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
            vec3s pos = GetProjectilePos(world, id, 1.0f, slot);
            vec3s dir = vecNorm(vecSub(cmd->aimpos, pos));

            { // Spawn fireball
                int fireball             = Sol_Prefab_Fireball(world, id, pos, dir, 30.0f, 1.0f);
                ScProjectile *projectile = Sol_Comp_Get(world, fireball, ScProjectile);
                projectile->hit          = (SolHit){
                    .entA   = id,
                    .damage = data->conf.damage,
                    .power  = data->power,
                };
                projectile->hit.damage.amount *= 0.5f;
                projectile->aoe_hit = (SolHit){
                    .entA   = id,
                    .damage = data->conf.damage,
                    .power  = data->power,
                };
                projectile->aoe_hit.damage.amount *= 0.5f;
                projectile->power = data->power;
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
    case 0:
        ScBody3 *body3 = Sol_Comp_Get(world, id, ScBody3);
        if (body3)
        {
            body3->vel = glms_vec3_lerpc(GLMS_VEC3_ZERO, body3->vel, 0.5f);
        }
        break;
    }
}

void Ability_Fireball_Enter(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot)
{
    AbilityStateData *data = &ability->stateData[slot];
    data->conf             = Sol_Ability_GetSlotConf(ability, slot);
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

void Ability_Fireball_Exit(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot)
{
    AbilityStateData *data  = &ability->stateData[slot];
    data->cooldownRemaining = data->conf.cooldown;
}

bool Ability_Fireball_CanExit(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot)
{
    AbilityStateData *data = &ability->stateData[slot];
    return true;
}

bool Ability_Fireball_CanEnter(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot)
{
    AbilityStateData *data = &ability->stateData[slot];
    return !(data->cooldownRemaining > 0.0f);
}

void Ability_Fireball_Draw(World *world, int id, ScAbility *ability, int slot, float dt)
{
    AbilityStateData *data = &ability->stateData[slot];
    ScCmd *cmd             = Sol_Comp_Get(world, id, ScCmd);

    vec3s pos = GetProjectilePos(world, id, data->power, slot);
    vec4s rot = {Sol_Quat_FromLookDir(cmd->aimdir).x, Sol_Quat_FromLookDir(cmd->aimdir).y,
                 Sol_Quat_FromLookDir(cmd->aimdir).z, Sol_Quat_FromLookDir(cmd->aimdir).w};

    switch (data->stage)
    {
    case 0: {
        *Sol_Render_GetNextSphere(PIPE_FIREBALL) = (SphereSSBO){
            .color = VEC4_RED,
            .pos   = (vec4s){pos.x, pos.y, pos.z, data->power * 0.5f},
        };
    }
    break;
    case 2: {
        *Sol_Render_GetNextModel(0, MODELKIND_CONE) = (ModelSSBO){
            .position = {pos.x, pos.y, pos.z, 1.0f},
            .rotation = rot,
            .scale    = {1.0f, 1.0f, 1.0f, 1.0f},
            .color    = {1, 0.2f, 0, 1},
        };
        Sol_Emitter_Push(world,
                         &(Emitter){
                             .kind        = EMITKIND_CONE,
                             .pos         = pos,
                             .dir         = cmd->aimdir,
                             .speed       = 20.0f,
                             .cone        = 0.2f,
                             .p_color     = {1, 0.1f, 0, 1},
                             .p_kind      = PARTICLE_FIRE,
                             .p_lifespan  = 0.5f,
                             .p_scale     = 0.5f,
                             .burst       = 2,
                             .scale_curve = CURVE_QUICKIN_SLOWOUT,
                             .alpha_curve = CURVE_QUICKIN_SLOWOUT,
                         },
                         1);
    }
    break;
    }
}

const AbilityStateFunc ability_fireball_state = {
    .update   = Spell,
    .enter    = Ability_Fireball_Enter,
    .exit     = Ability_Fireball_Exit,
    .canExit  = Ability_Fireball_CanExit,
    .canEnter = Ability_Fireball_CanEnter,
    .draw     = Ability_Fireball_Draw,
};
const AbilityStateFunc ability_fireball_charge_state = {
    .update   = Charge,
    .enter    = Ability_Fireball_Enter,
    .exit     = Ability_Fireball_Exit,
    .canExit  = Ability_Fireball_CanExit,
    .canEnter = Ability_Fireball_CanEnter,
    .draw     = Ability_Fireball_Draw,
};
const AbilityStateFunc ability_fireball_dash_state = {
    .update   = Dash,
    .enter    = Ability_Fireball_Enter,
    .exit     = Ability_Fireball_Exit,
    .canExit  = Ability_Fireball_CanExit,
    .canEnter = Ability_Fireball_CanEnter,
    .draw     = Ability_Fireball_Draw,
};
