#include "s_combat.h"
#include "sol_core.h"
#include "world.h"
#include "model/s_model.h"
#include "ability/s_ability.h"

static const int model_map[ABILITY_STATE_COUNT] = {
    [ABILITY_STATE_IDLE] = -1,
    [ABILITY_STATE_CLAW] = MODELKIND_WEAPONBLADE,
    [ABILITY_STATE_DASH] = -1,
    [ABILITY_STATE_FIREBALL] = -1,
    [ABILITY_STATE_PISTOL] = -1,
    [ABILITY_STATE_SPINSLASH] = -1,
    [ABILITY_STATE_SHIELD] = -1,
    [ABILITY_STATE_LASER] = -1,
    [ABILITY_STATE_WHIP] = -1,
    [ABILITY_STATE_FIREBALLVOLLEY] = -1,
};

// void Weapon_Step(World *world, double dt, double time)
// {
//     for (int i = 0; i < world->activeCount; i++)
//     {
//         int id = world->activeEntities[i];
//         if (!(world->masks[id] & HAS_COMBAT))
//             continue;
//         CompCombat *combat         = &world->combats[id];
//         CompXform  *leftWeapXform  = &world->xforms[combat->leftWeaponEnt];
//         CompXform  *rightWeapXform = &world->xforms[combat->rightWeaponEnt];

//         SolXform leftXform  = Sol_Model_GetBoneXform(world, id, "hand.L");
//         leftWeapXform->pos  = leftXform.pos;
//         leftWeapXform->quat = leftXform.rot;

//         SolXform rightXform  = Sol_Model_GetBoneXform(world, id, "hand.R");
//         rightWeapXform->pos  = rightXform.pos;
//         rightWeapXform->quat = rightXform.rot;
//     }
// }

void Sol_Weapon_Equip(World *world, int id, int ability, int slot)
{
    CompCombat *combat = &world->combats[id];
    if (slot == 0)
    {
        if (!combat->leftWeaponEnt)
        {
            combat->leftWeaponEnt = Sol_Create_Ent(world, 0);
            Sol_Model_Add(world, combat->leftWeaponEnt, MODELKIND_WEAPONBLADE, 0);
        }
        Sol_Model_SetModelId(world, combat->leftWeaponEnt, model_map[ability]);
    }
    if (slot == 1)
    {
        if (!combat->rightWeaponEnt)
        {
            combat->rightWeaponEnt = Sol_Create_Ent(world, 0);
            Sol_Model_Add(world, combat->rightWeaponEnt, MODELKIND_WEAPONBLADE, 0);
        }
        Sol_Model_SetModelId(world, combat->rightWeaponEnt, model_map[ability]);
    }
}