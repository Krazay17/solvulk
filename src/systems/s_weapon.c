/*
 * File: s_weapon.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-10-06
 *
 */
#include "world.h"
#include "sol_math.h"

int Sol_Weapon_Spawn(World *world, int owner, WeaponKind kind, const char *bone)
{
    int id           = Sol_Create_Ent(world, GLMS_VEC3_ZERO);
    ScWeapon *weapon = Sol_Comp_Add(world, id, ScWeapon);
    ScModel *model   = Sol_Comp_Add(world, id, ScModel);
    model->kind      = MODELKIND_SCYTHE;
    ScParent *parent = Sol_Comp_Add(world, id, ScParent);
    parent->parentId = owner;
    memcpy(parent->boneFollow, bone, sizeof(parent->boneFollow));
    glm_euler_zyx_quat((vec3){90.0f, 0.0f, 0.0f}, parent->localQuat.raw);


    return id;
}

vec3s Sol_Weapon_DmgPos(World *world, int id)
{
    return Sol_Model_GetBoneXform(world, id, "Blade").pos;
}