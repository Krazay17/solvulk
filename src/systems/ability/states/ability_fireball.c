#include "world.h"
#include "estate.h"
#include "sol_core.h"
#include "sol_math.h"

#include "render/render.h"
#include "prefabs.h"

#define MIN_POWER 0.2f
#define MAX_POWER 3.0f

static vec3s GetProjectilePos(World *world, int id, ScCmd *cmd, float power)
{
    vec3s pos = Sol_Model_GetBoneXform(world, id, "hand.L").pos;
    if (glms_vec3_norm2(pos) == 0.0f)
        pos = Sol_Body3_GetHead(world, id);
    pos = vecAdd(pos, vecSca(cmd->aimdir, (power - (MIN_POWER + 0.2f))));
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
        data->power = Sol_Math_Lerp_Clamped(MIN_POWER, MAX_POWER, data->elapsed / data->duration);

        if (!data->held)
            data->stage++;
        break;
    case 1:
        data->stage++;
        vec3s pos = GetProjectilePos(world, id, cmd, data->power);
        vec3s dir = vecNorm(vecSub(cmd->aimpos, pos));
        
        { // Spawn fireball
            int fireball             = Sol_Prefab_Fireball(world, id, pos, dir, 20.0f, data->power);
            ScProjectile *projectile = Sol_Comp_Get(world, fireball, ScProjectile);
            projectile->hit          = (SolHit){
                .entA       = id,
                .effectMask = ability_base[ABILITY_STATE_FIREBALL].effectMask,
                .damage     = 10.0f,
            };
            projectile->power = data->power;
        }
        break;
    case 2:
        data->recover += dt;
        if (data->recover > data->recoverDuration)
            Sol_Ability_SetState(world, id, 0, 0, true);
        break;
    }
}

void Ability_Fireball_Enter(World *world, int id, ScAbility *ability, ScCmd *cmd)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];
    data->duration         = ability_base[ABILITY_STATE_FIREBALL].duration;
    data->cooldown         = ability_base[ABILITY_STATE_FIREBALL].cooldown;
    data->recoverDuration  = ability_base[ABILITY_STATE_FIREBALL].recoverDuration;

    Sol_Comp_Get(world, id, ScCombat)->hitPause = 0;
}

void Ability_Fireball_Exit(World *world, int id, ScAbility *ability, ScCmd *cmd)
{
    AbilityStateData *data  = &ability->stateData[ability->activeSlot];
    data->cooldownRemaining = data->cooldown;
}

bool Ability_Fireball_CanExit(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 next)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];
    return data->elapsed > data->duration * 0.8f;
}

bool Ability_Fireball_CanEnter(World *world, int id, ScAbility *ability, ScCmd *cmd, u32 last, int slot)
{
    AbilityStateData *data = &ability->stateData[slot];
    return data->cooldownRemaining <= 0.0f;
}

void Ability_Fireball_Draw(World *world, int id, ScAbility *ability, ScCmd *cmd)
{
    AbilityStateData *data = &ability->stateData[ability->activeSlot];

    if (data->stage > 0)
        return;

    vec3s pos = GetProjectilePos(world, id, cmd, data->power);

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
