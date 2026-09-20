#include "world.h"
#include "estate.h"
#include "sol_core.h"
#include "sol_math.h"

#include "render/render.h"
#include "prefabs.h"

#define MIN_POWER 0.2f
#define MAX_POWER 2.5f

static vec3s GetProjectilePos(World *world, int id, float power)
{
    vec3s lookdir = Sol_RotFromQuat(Xform_GetDraw(world, id).rot);
    vec3s pos     = Sol_Model_GetBoneXform(world, id, "hand.L").pos;
    if (glms_vec3_norm2(pos) == 0.0f)
        pos = Sol_Body3_GetHead(world, id);
    pos = vecAdd(pos, vecSca(lookdir, (power - (MIN_POWER + 0.2f))));
    pos = vecAdd(pos, vecSca(WORLD_UP, (power - (MIN_POWER + 0.6f))));
    return pos;
}

void Ability_Fireball_Update(World *world, int id, ScAbility *ability, ScCmd *cmd, float dt)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];
    data->elapsed += dt;
    switch (data->stage)
    {
    case 0:
        data->power = Sol_Math_Lerp_Clamped(MIN_POWER, data->conf.maxpower, data->elapsed / data->conf.duration);

        if (!data->held)
            data->stage++;
        break;
    case 1:
        data->stage++;
        vec3s pos = GetProjectilePos(world, id, data->power);
        vec3s dir = vecNorm(vecSub(cmd->aimpos, pos));

        { // Spawn fireball
            int fireball             = Sol_Prefab_Fireball(world, id, pos, dir, 20.0f, data->power);
            ScProjectile *projectile = Sol_Comp_Get(world, fireball, ScProjectile);
            projectile->hit          = (SolHit){
                .entA       = id,
                .damage     = data->conf.damage,
            };
            projectile->aoe_hit = (SolHit){
                .entA       = id,
                .effectMask = data->conf.effectMask,
                .buffMask   = data->conf.buffMask,
                .damage     = data->conf.damage,
            };
            projectile->power = data->power;
        }
        break;
    case 2:
        data->recoverRemaining += dt;
        if (data->recoverRemaining > data->conf.recover)
            Sol_Ability_SetState(world, id, 0, 0, true);
        break;
    }
}

void Ability_Fireball_Enter(World *world, int id, ScAbility *ability, ScCmd *cmd)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];
    data->conf             = Sol_Ability_GetSlotConf(ability, ability->activeSlot);
    
    data->cooldownRemaining = data->conf.cooldown;
}

void Ability_Fireball_Exit(World *world, int id, ScAbility *ability, ScCmd *cmd)
{
    AbilityStateData *data  = &ability->stateData[ability->activeSlot];
}

bool Ability_Fireball_CanExit(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 next)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];
    return data->elapsed > data->conf.duration * 0.8f;
}

bool Ability_Fireball_CanEnter(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 last, int slot)
{
    AbilityStateData *data = &ability->stateData[slot];
    return data->cooldownRemaining <= 0.0f;
}

void Ability_Fireball_Draw(World *world, int id, ScAbility *ability)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];

    if (data->stage > 0)
        return;

    vec3s pos = GetProjectilePos(world, id, data->power);

    *Sol_Render_GetNextSphere(PIPE_FIREBALL) = (SphereSSBO){
        .color = VEC4_RED,
        .pos   = (vec4s){pos.x, pos.y, pos.z, data->power},
    };
}

extern const AbilityStateFunc ability_fireball_state = {
    .update   = Ability_Fireball_Update,
    .enter    = Ability_Fireball_Enter,
    .exit     = Ability_Fireball_Exit,
    .canExit  = Ability_Fireball_CanExit,
    .canEnter = Ability_Fireball_CanEnter,
    .draw     = Ability_Fireball_Draw,
};
