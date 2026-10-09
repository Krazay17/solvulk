/*
 * File: s_weapon.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-10-06
 *
 */
#include "world.h"
#include "sol_math.h"

const ScWeapon weapon_kinds[WEAPONKIND_COUNT] = {
    [WEAPONKIND_CLAW] =
        {
            .kind = WEAPONKIND_CLAW,
        },
};

int Make_Claw(World *world, int owner, int slot)
{
    char *bone       = slot > 5 ? "hand.R.Weapon" : "hand.L.Weapon";
    int id           = Sol_Create_Ent(world, GLMS_VEC3_ZERO);
    ScWeapon *weapon = Sol_Comp_Add(world, id, ScWeapon);
    ScParent *parent = Sol_Comp_Add(world, id, ScParent);
    parent->parentId = owner;

    memcpy(parent->boneFollow, bone, sizeof(parent->boneFollow));
    parent->localQuat = GLMS_QUAT_IDENTITY;

    ScModel *model = Sol_Comp_Add(world, id, ScModel);
    model->kind    = MODELKIND_WEAPONBLADE;
    Sol_Comp_Add(world, id, ScAnim);
    ScRibbon *ribbon = Sol_Ribbon_AddKind(world, id, RIBBONKIND_WEAPON_TRAIL);
    ribbon->rate     = 0.01f;
    return id;
}

int Make_Scythe(World *world, int owner, int slot)
{
    char *bone       = slot > 5 ? "hand.R.Weapon" : "hand.L.Weapon";
    int id           = Sol_Create_Ent(world, GLMS_VEC3_ZERO);
    ScWeapon *weapon = Sol_Comp_Add(world, id, ScWeapon);
    ScParent *parent = Sol_Comp_Add(world, id, ScParent);
    parent->parentId = owner;

    memcpy(parent->boneFollow, bone, sizeof(parent->boneFollow));
    glm_euler_zyx_quat((vec3){glm_rad(90.0f), 0.0f, 0.0f}, parent->localQuat.raw);

    ScModel *model   = Sol_Comp_Add(world, id, ScModel);
    model->kind      = MODELKIND_SCYTHE;
    ScRibbon *ribbon = Sol_Ribbon_AddKind(world, id, RIBBONKIND_WEAPON_TRAIL_COLORRING);
    ribbon->rate     = 0.01f;

    return id;
}

void Weapon_Update(World *world, double dt)
{
    SparseSet_ScWeapon *set = Sol_Comp_Set(world, ScWeapon);
    for (int i = 0; i < set->cnt; i++)
    {
        int id       = set->dense[i];
        ScWeapon *sc = &set->data[i];

        Xform blade_xform = Sol_Weapon_BladeXform(world, id);
        if (sc->update_trail)
        {
            ScRibbon *ribbon = Sol_Comp_Get(world, id, ScRibbon);
            if (ribbon)
            {
                ribbon->_accum += dt;
                while (ribbon->_accum >= ribbon->rate)
                {
                    ribbon->_accum -= ribbon->rate;
                    float spin = Sol_QuatGetRoll(blade_xform.rot, WORLD_FWD, WORLD_UP);

                    spin += glm_rad(90.0f);
                    Sol_Ribbon_Addpoint(&ribbon->ribbon, blade_xform.pos, 0, spin);
                }
            }
        }
    }
}

const MakeWeapon Make_Weapon[WEAPONKIND_COUNT] = {
    [WEAPONKIND_CLAW]   = Make_Claw,
    [WEAPONKIND_SCYTHE] = Make_Scythe,
};

vec3s Sol_Weapon_DmgPos(World *world, int id)
{
    return Sol_Model_GetBoneXform(world, id, "Blade").pos;
}

Xform Sol_Weapon_BladeXform(World *world, int id)
{
    return Sol_Model_GetBoneXform(world, id, "Blade");
}
