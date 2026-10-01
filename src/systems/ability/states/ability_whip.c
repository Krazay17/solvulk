#include "world.h"
#include "sol_math.h"
#include "estate.h"
#include "prefabs.h"

const float accel     = 12.0f; // speed gained per second
const float min_speed = 12.0f; // so you can start from standstill

static void BoltDelete(World *world, AbilityStateData *data)
{
    if (data->as.whip.bolt > 0)
    {
        Sol_Destroy_Ent(world, data->as.whip.bolt);
        data->as.whip.bolt     = 0;
        data->as.whip.bolt_hit = 0;
    }
}

static void BoltHit(World *world, int a, int b)
{
    ScRef *ref = Sol_Comp_Get(world, a, ScRef);

    ScAbility *ability = Sol_Comp_Get(world, ref->ent_id, ScAbility);
    if (!ability || ability->state != ABILITY_STATE_WHIP_CHARGE)
        return;

    AbilityStateData *data = &ability->stateData[ref->index];
    data->as.whip.bolt_hit = true;
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
        data->power = minf(data->conf.maxpower, data->power + dt * data->conf.speed);

        ScBody3 *body3 = Sol_Comp_Get(world, id, ScBody3);
        ScMove3 *move3 = Sol_Comp_Get(world, id, ScMove3);
        if (body3 && move3 && data->as.whip.bolt_hit)
        {
            move3->frictionMod = 0.0f;
            vec3s pos          = world->xform.pos[id];
            vec3s bolt_pos     = world->xform.pos[data->as.whip.bolt];
            vec3s delta        = glms_vec3_sub(bolt_pos, pos);
            float d2           = glms_vec3_norm2(delta);
            if (d2 > 0.001f)
            {
                vec3s to_bolt = glms_vec3_normalize(delta);
                vec3s look    = cmd->lookdir;
                float align   = glms_vec3_dot(to_bolt, look);
                vec3s axis    = glms_vec3_cross(to_bolt, look);
                vec3s tangent = glms_vec3_cross(axis, to_bolt);
                vec3s v_side  = glms_vec3_sub(body3->vel, glms_vec3_scale(to_bolt, glms_vec3_dot(body3->vel, to_bolt)));
                tangent       = glms_vec3_norm2(v_side) > 0.0001f ? glms_vec3_normalize(v_side) : to_bolt;

                float t = align > 0 ? align : 0;
                t       = t * t * t;

                vec3s target_dir = glms_vec3_normalize(glms_vec3_lerpc(tangent, to_bolt, t));
                float speed      = glms_vec3_norm(body3->vel);
                speed            = fmaxf(speed, min_speed) + accel * dt;
                body3->vel       = glms_vec3_scale(target_dir, speed);
            }

            if (d2 < 10.0f)
            {
                BoltDelete(world, data);
            }
        }
        break;
    case 1:
    fire:
        break;
    }
    if (data->stage > 0)
    {
        data->elapsed += dt;
        if (data->elapsed >= data->conf.duration)
            Sol_Ability_SetState(world, id, 0, ability->activeSlot, true);
    }
}
static void Enter(World *world, int id, ScAbility *ability, ScCmd *cmd)
{
    AbilityStateData *data  = &ability->stateData[ability->activeSlot];
    data->conf              = Sol_Ability_GetSlotConf(ability, ability->activeSlot);
    data->cooldownRemaining = data->conf.cooldown;

    int bolt           = Sol_Prefab_LightningBolt(world, id, Sol_Body3_GetHead(world, id), cmd->aimdir, 80.0f, BoltHit);
    ScRef *ref         = Sol_Comp_Add(world, bolt, ScRef);
    ref->kind          = REFKIND_ABILITY;
    ref->index         = ability->activeSlot;
    ref->ent_id        = id;
    data->as.whip.bolt = bolt;
    ScProjectile *p    = Sol_Comp_Get(world, bolt, ScProjectile);
    p->hit.damage      = data->conf.damage;
    p->hit.power       = 1.0f;
}
static void Exit(World *world, int id, ScAbility *ability, ScCmd *cmd)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];
    BoltDelete(world, data);
}
static bool CanExit(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 next)
{
    return true;
}
static bool CanEnter(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 last, int slot)
{
    AbilityStateData *data = &ability->stateData[slot];
    return !(data->cooldownRemaining > 0.0f);
}
static DefendResult Defend(World *world, int id, ScAbility *ability, SolHit *hit)
{
}
static void Draw(World *world, int id, ScAbility *ability, float dt)
{
}

const AbilityStateFunc ability_whip_charge = {Charge, Enter, Exit, CanExit, CanEnter, Draw, Defend};