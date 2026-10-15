/*
 * File: s_equip.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-10-06
 *
 */
#include "world.h"
#include "sol_math.h"

const WeaponKind item_weapon_map[ITEMKIND_COUNT] = {
    [ITEMKIND_CLAW]   = WEAPONKIND_CLAW,   //
    [ITEMKIND_SCYTHE] = WEAPONKIND_SCYTHE, //
};

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
            forc(world, id, ScRibbon)
            {
                c->_accum += dt;
                while (c->_accum >= c->rate)
                {
                    c->_accum -= c->rate;
                    float spin = Sol_QuatGetRoll(blade_xform.rot, WORLD_FWD, WORLD_UP);

                    spin += glm_rad(90.0f);
                    Sol_Ribbon_Addpoint(&c->ribbon, blade_xform.pos, 0, spin);
                }
            }
        }
        if (!sc->extending)
        {
            forc(world, id, ScParent) c->localScale.y = Sol_InterpToOne(c->localScale.y, dt);
        }
    }
}

int Sol_Equip_Item(World *world, int owner, u32 item_kind, int slot)
{
    if (slot < 5)
        return 0;

    ScEquip *equip = Sol_Comp_Add(world, owner, ScEquip);
    int *weaponId  = slot > 5 ? &equip->right_weapon : &equip->left_weapon;

    if (*weaponId > 0)
    {
        Sol_Destroy_Ent(world, *weaponId);
        *weaponId = -1;
    }
    if (!item_kind)
        return -1;

    int id    = Sol_Create_Ent(world, GLMS_VEC3_ZERO);
    *weaponId = id;

    WeaponKind kind       = item_weapon_map[item_kind];
    ScWeapon *weapon      = Sol_Comp_Add(world, id, ScWeapon);
    weapon->kind          = kind;
    weapon->equipped_slot = slot;
    ScOwner *owner_comp   = Sol_Comp_Add(world, id, ScOwner);
    owner_comp->ownerId   = owner;
    ScParent *parent      = Sol_Comp_Add(world, id, ScParent);
    parent->parentId      = owner;

    char *bone = slot > 5 ? "weapon.R" : "weapon.L";
    memcpy(parent->boneFollow, bone, sizeof(parent->boneFollow));
    parent->localQuat  = GLMS_QUAT_IDENTITY;
    parent->localScale = (vec3s){1, 1, 1};

    ScRibbon *ribbon       = Sol_Ribbon_AddKind(world, id, RIBBONKIND_WEAPON_TRAIL);
    ribbon->rate           = 0.01f;
    ribbon->ribbon.texture = SOL_TEXTURE_WEAPONTRAIL;

    switch (kind)
    {
    case WEAPONKIND_CLAW: {
        ScModel *model = Sol_Comp_Add(world, id, ScModel);
        model->kind    = MODELKIND_WEAPONBLADE;
        Sol_Comp_Add(world, id, ScAnim);
    }
    break;
    case WEAPONKIND_SCYTHE: {
        ScModel *model         = Sol_Comp_Add(world, id, ScModel);
        model->kind            = MODELKIND_SCYTHE;
        ribbon->ribbon.texture = SOL_TEXTURE_SQUARERING;
        glm_euler_zyx_quat((vec3){glm_rad(90.0f), 0.0f, glm_rad(180.0f)}, parent->localQuat.raw);
    }
    break;
    }

    return id;
}

vec3s Sol_Weapon_DmgPos(World *world, int id)
{
    return Sol_Model_GetBoneXform(world, id, "Blade").pos;
}

Xform Sol_Weapon_BladeXform(World *world, int id)
{
    return Sol_Model_GetBoneXform(world, id, "Blade");
}
void Sol_Weapon_Extend(World *world, int id, float power)
{
    forc(world, id, ScParent) c->localScale.y = 1.0f + (power);
    forc(world, id, ScWeapon) c->extending    = true;
}
void Sol_Weapon_Unextend(World *world, int id)
{
    forc(world, id, ScEquip)
    {
        int left  = c->left_weapon;
        int right = c->right_weapon;

        forc(world, left, ScWeapon) c->extending  = false;
        forc(world, right, ScWeapon) c->extending = false;
    }
}
int Sol_Equip_GetId(World *world, int id, int slot)
{
    forc(world, id, ScEquip) return slot > 5 ? c->right_weapon : c->left_weapon;
    return 0;
}
void Sol_Weapon_TrailOn(World *world, int id, bool is_right)
{
    forc(world, id, ScEquip)
    {
        int weaponId = is_right ? c->right_weapon : c->left_weapon;
        forc(world, weaponId, ScWeapon)
        {
            c->update_trail = true;
        }
    }
}
void Sol_Weapon_TrailsOff(World *world, int id)
{
    forc(world, id, ScEquip)
    {
        int left  = c->left_weapon;
        int right = c->right_weapon;

        forc(world, left, ScWeapon) c->update_trail  = false;
        forc(world, right, ScWeapon) c->update_trail = false;
    }
}

SolRay Sol_Weapon_Ray(World *world, int id, int right_hand, float power)
{
    SolRay ray;
    const char *bone = right_hand ? "weapon.R" : "weapon.L";
    Xform hand_xform = Sol_Model_GetBoneXform(world, id, bone);
    forc(world, id, ScEquip)
    {
        Xform blade_xform = Sol_Weapon_BladeXform(world, right_hand ? c->right_weapon : c->left_weapon);
        SolDistDir dd     = Sol_GetDistDir(blade_xform.pos, hand_xform.pos);
        ray.dir           = dd.dir;
        ray.dist          = dd.dist;
        ray.start         = hand_xform.pos;
        ray.mask          = COLLAYER_ALL;
        ray.radius        = 0.4f;
        ray.ignoreEnt     = id;
        return ray;
    }

    return ray;
}