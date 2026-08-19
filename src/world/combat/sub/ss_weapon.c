#include "combat/i_combat.h"

#include "parent/s_parent.h"
#include "sol_core.h"
#include "world.h"
#include "model/s_model.h"
#include "replication/s_replication.h"
#include "ability/s_ability.h"

static const int model_map[ABILITY_STATE_COUNT] = {
    [ABILITY_STATE_IDLE] = -1,   [ABILITY_STATE_CLAW] = MODELKIND_WEAPONBLADE,
    [ABILITY_STATE_DASH] = -1,   [ABILITY_STATE_FIREBALL] = -1,
    [ABILITY_STATE_PISTOL] = -1, [ABILITY_STATE_SPINSLASH] = -1,
    [ABILITY_STATE_SHIELD] = -1, [ABILITY_STATE_LASER] = -1,
    [ABILITY_STATE_WHIP] = -1,   [ABILITY_STATE_FIREBALLVOLLEY] = -1,
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
    CompCombat *combat   = &world->combats[id];
    int        *weaponId = slot == 0 ? &combat->leftWeaponEnt : &combat->rightWeaponEnt;
    char       *bone     = {slot == 0 ? "hand.L.Weapon" : "hand.R.Weapon"};
    if (!ability && *weaponId != -1)
    {
        Sol_Destroy_Ent(world, *weaponId);
        *weaponId = -1;
    }
    else if (*weaponId == -1)
    {
        *weaponId = Sol_Create_Ent(world, 0);
        Sol_Model_Add(world, *weaponId, model_map[ability]);
        CompParent *parent = Sol_Parent_Add(world, *weaponId, id);
        memcpy(parent->boneFollow, bone, 16);

        // Sol_Replication_Add(world, *weaponId, NETAUTH_AUTH, 0);
    }
}