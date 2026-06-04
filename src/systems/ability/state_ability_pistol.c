#include "sol_core.h"

#include "ability_i.h"

static bool LeaveState(World *world, int id)
{
}

void Pistol_State_Update(World *world, int id, float dt)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ability->activeSlot];
    if (!data->held)
    {
        Sol_Ability_SetState(world, id, ABILITY_STATE_IDLE, 0,false);
        return;
    }
    data->elapsed += dt;
    data->accum += dt;

    if (data->accum > 0.05f)
    {
        data->accum = 0;

        SolShoot shoot  = Sol_Controller_GetShoot(world, id, 150.0f);
        int      bullet = Sol_Prefab_Factory(world, 0, ENTKIND_BULLET,
                                             (EntDesc){
                                                 .pos   = shoot.pos,
                                                 .scale = 0.15f,
                                             });
        if (bullet > 0)
        {
            Sol_Physx_SetVel(world, bullet, shoot.vel);
            Sol_Owner_Add(world, bullet, id);
        }
        Sol_Model_PlayAnim(world, id,
                           (AnimDesc){
                               .anim    = ANIM_ATTACK_RIGHT,
                               .speed   = 3.5f,
                               .oneShot = true,
                               .force   = true,
                               .seek    = 0.1,
                               .layerId = ANIM_LAYER_UPPER,
                               .blendIn = 0.01,
                           });
    }
}
void Pistol_State_Enter(World *world, int id)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ability->activeSlot];
    data->accum          = 0.2f;
}
void Pistol_State_Exit(World *world, int id)
{
    Sol_Model_PlayAnim(world, id, (AnimDesc){.layerId = ANIM_LAYER_UPPER});
}
bool Pistol_State_CanExit(World *world, int id, u32 next)
{
    return next != ABILITY_STATE_PISTOL;
}
bool Pistol_State_CanEnter(World *world, int id, u32 last, u32 next)
{
    return true;
}
