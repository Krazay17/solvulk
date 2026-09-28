/*
 * File: ability_fireball.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-28
 *
 */
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

static vec3s GetProjectilePos(World *world, int id, float power, int slot)
{
    vec3s pos = Sol_Model_GetBoneXform(world, id, slot == 1 ? "hand.R" : "hand.L").pos;
    if (glms_vec3_norm2(pos) == 0.0f)
        pos = Sol_Body3_GetHead(world, id);
    pos = vecAdd(pos, vecSca(WORLD_UP, (power * 0.5f)));
    return pos;
}

static void Dash(World *world, int id, ScAbility *ability, ScCmd *cmd, float dt)
{
}

static void Charge(World *world, int id, ScAbility *ability, ScCmd *cmd, float dt)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];
    switch (data->stage)
    {
    case 0:
        if (!data->held)
        {
            data->stage++;
            goto fire;
        }
        data->power = min(data->conf.maxpower, data->power + (dt * data->conf.chargespeed));
        break;
    case 1:
    fire:
        data->stage++;
        vec3s pos = GetProjectilePos(world, id, data->power, ability->activeSlot);
        vec3s dir = vecNorm(vecSub(cmd->aimpos, pos));

        SolHit hit = {
            .entA   = id,
            .power  = data->power,
            .damage = data->conf.damage,
        };

        { // Spawn fireball
            int fireball                   = Sol_Prefab_Fireball(world, id, pos, dir, 25.0f, data->power);
            ScBody3 *pBody                 = Sol_Comp_Get(world, fireball, ScBody3);
            ScProjectile *projectile       = Sol_Comp_Get(world, fireball, ScProjectile);
            projectile->hit                = hit;
            projectile->aoe_hit            = hit;
            projectile->aoe_hit.effectMask = data->conf.effectMask;
            projectile->aoe_hit.buffMask   = data->conf.buffMask;
            projectile->power              = data->power;
        }
    case 2:
        if (data->elapsed > HIT_DELAY)
            data->stage++;
        break;
    case 3:
        data->accum += dt;
        if (data->accum >= HIT_RATE)
        {
            data->accum -= HIT_RATE;
            vec3s pos = GetProjectilePos(world, id, data->power, ability->activeSlot);
            vec3s dir = vecNorm(vecSub(cmd->aimpos, pos));

            SolHit hit = {
                .entA       = id,
                .kind       = HITKIND_MELEE_HIT,
                .power      = data->power,
                .damage     = data->conf.damage,
                .power      = 0.3f,
                .buffMask   = BITC(BUFFKIND_FIRE),
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

    if (data->stage > 0)
        data->elapsed += dt;
    if (data->elapsed >= data->conf.duration)
    {
        Sol_Ability_SetState(world, id, 0, ability->activeSlot, true);
        return;
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

void Ability_Fireball_Enter(World *world, int id, ScAbility *ability, ScCmd *cmd)
{
    AbilityStateData *data  = &ability->stateData[ability->activeSlot];
    data->conf              = Sol_Ability_GetSlotConf(ability, ability->activeSlot);
    data->hitgen            = Sol_Hitgen_Start(world, id);
    data->cooldownRemaining = data->conf.cooldown;
    data->power             = MIN_POWER;
}

void Ability_Fireball_Exit(World *world, int id, ScAbility *ability, ScCmd *cmd)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];
}

bool Ability_Fireball_CanExit(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 next)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];
    return true;
}

bool Ability_Fireball_CanEnter(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 last, int slot)
{
    AbilityStateData *data = &ability->stateData[slot];
    return !(data->cooldownRemaining > 0.0f);
}

void Ability_Fireball_Draw(World *world, int id, ScAbility *ability, float dt)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];
    ScCmd *cmd             = Sol_Comp_Get(world, id, ScCmd);

    vec3s pos = GetProjectilePos(world, id, data->power, ability->activeSlot);
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
    case 3: {
        // *Sol_Render_GetNextSphere(PIPE_PLASMA) = (SphereSSBO){
        //     .color = VEC4_RED,
        //     .pos   = (vec4s){pos.x, pos.y, pos.z, 0.25f},
        // };
        *Sol_Render_GetNextModel(0, MODELKIND_CONE) = (ModelSSBO){
            .position = {pos.x, pos.y, pos.z, 1.0f},
            .rotation = rot,
            .scale    = {1.0f, 1.0f, 1.0f, 1.0f},
            .color    = {1, 0.1f, 0, 1},
        };
        Sol_Emitter_Push(world,
                         &(Emitter){
                             .kind        = EMITKIND_CONE,
                             .pos         = pos,
                             .dir         = cmd->aimdir,
                             .speed       = 15.0f,
                             .cone        = 0.2f,
                             .p_color     = {1, 0.1f, 0, 1},
                             .p_kind      = PARTICLE_FIRE,
                             .p_lifespan  = 0.5f,
                             .p_scale     = 0.5f,
                             .burst       = 1,
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
