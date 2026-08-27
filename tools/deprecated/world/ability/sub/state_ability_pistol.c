#include "ability/si_ability.h"
#include "sol_core.h"
#include "world.h"
#include "sol_math.h"
#include "audio.h"
#include "xform/s_xform.h"
#include "controller/s_controller.h"
#include "combat/s_combat.h"
#include "model/s_model.h"
#include "event/s_event.h"
#include "physx/s_body.h"
#include "movement/s_movement.h"
#include "game/prefabs.h"
#include "projectile/s_projectile.h"


void Pistol_State_Update(World *world, int id, float dt)
{
    CompAbility *ability = Sol_Ability_Get(world, id);
    AbilityStateData *data    = &ability->stateData[ability->activeSlot];
    if (!data->held)
    {
        Sol_Ability_SetState(world, id, ABILITY_STATE_IDLE, 0, false);
        return;
    }
    data->elapsed += dt;
    data->accum += dt;

    if (data->accum > ability_base[ABILITY_STATE_PISTOL].cooldown)
    {
        data->accum = 0;

        SolShoot shoot  = Sol_Controller_GetShoot(world, id, 1.0f, 150.0f);
        int      bullet = Sol_Prefab_Factory(world, 0, EKIND_BULLET,
                                             (EntDesc){
                                                 .pos   = shoot.pos,
                                                 .scale = 0.15f,
                                             });
        if (bullet > 0)
        {
            Sol_Physx_SetVel(world, bullet, shoot.vel);
            Sol_Owner_Add(world, bullet, id);
            world->projectiles[bullet].directHit.damage = ability_base[ABILITY_STATE_PISTOL].damage;
            world->projectiles[bullet].directHit.buffMask = ability_base[ABILITY_STATE_PISTOL].buffMask;
            world->projectiles[bullet].directHit.effectMask = ability_base[ABILITY_STATE_PISTOL].effectMask;
        }
        Sol_Model_PlayAnim(world, id,
                           (AnimDesc){
                               .anim    = ANIM_ATTACK_RIGHT,
                               .speed   = 3.5f,
                               .playKind = ANIMPLAYKIND_ONESHOT,
                               .seek    = 0.15f,
                               .layerId = ANIM_LAYER_UPPER,
                               .blendIn = 0.01f,
                           });
    }
}
void Pistol_State_Enter(World *world, int id)
{
    CompAbility *ability = Sol_Ability_Get(world, id);
    AbilityStateData *data    = &ability->stateData[ability->activeSlot];
    data->accum          = ability_base[ABILITY_STATE_PISTOL].cooldown;
}
void Pistol_State_Exit(World *world, int id)
{
    CompAbility *ability = Sol_Ability_Get(world, id);
    AbilityStateData *data    = &ability->stateData[ability->activeSlot];
    data->lastExited     = solState.gameTime;
    Sol_Model_StopAnim(world, id, ANIM_LAYER_UPPER, 0);
}
bool Pistol_State_CanExit(World *world, int id, u32 next)
{
    CompAbility *ability = Sol_Ability_Get(world, id);
    AbilityStateData *data    = &ability->stateData[ability->activeSlot];

    return data->elapsed >= ability_base[ABILITY_STATE_PISTOL].duration;
}
bool Pistol_State_CanEnter(World *world, int id, u32 last, u32 next, int slot)
{
    CompAbility *ability = Sol_Ability_Get(world, id);
    AbilityStateData *data    = &ability->stateData[slot];
    return slot != ability->activeSlot && !(data->lastExited + ability_base[ABILITY_STATE_PISTOL].cooldown > solState.gameTime);
}
