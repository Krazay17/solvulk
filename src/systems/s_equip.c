/*
 * File: s_equip.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-10-06
 *
 */
#include "world.h"
#include "sol_math.h"
#include "model.h"
#include "ribbon/s_ribbon.h"

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

        Xform trail_xform = Sol_Model_GetBoneXform(world, id, "Trail");
        if (sc->update_trail)
        {
            forc(world, id, ScRibbon)
            {
                c->_accum += dt;
                while (c->_accum >= c->rate)
                {
                    c->_accum -= c->rate;
                    vec3s side = glms_quat_rotatev(trail_xform.rot, WORLD_DOWN);
                    Sol_Ribbon_Addpoint(&c->ribbon, trail_xform.pos, side);
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
    parent->localScale = (vec3s){1, 0, 1};

    ScRibbon *ribbon       = Sol_Ribbon_AddKind(world, id, RIBBONKIND_WEAPON_TRAIL);
    ribbon->rate           = 0.01f;
    ribbon->ribbon.texture = SOL_TEXTURE_WEAPONTRAIL;

    ScModel *model = Sol_Comp_Add(world, id, ScModel);
    switch (kind)
    {
    case WEAPONKIND_CLAW: {
        model->kind = MODELKIND_WEAPONBLADE;
        Sol_Comp_Add(world, id, ScAnim);
    }
    break;
    case WEAPONKIND_SCYTHE: {
        model->kind                 = MODELKIND_SCYTHE;
        ribbon->ribbon.thickness[0] = 1.3f;
        ribbon->ribbon.texture      = SOL_TEXTURE_SQUARERING;
        glm_euler_zyx_quat((vec3){glm_rad(90.0f), 0.0f, glm_rad(180.0f)}, parent->localQuat.raw);
    }
    break;
    default:
        model->kind        = MODELKIND_WEAPONBLADE;
        model->transparent = true;
    }
    SolSkeleton *skeleton = &loaded_models[model->kind].skeleton;
    for (int i = 0; i < skeleton->boneCount; i++)
    {
        char blade_name[] = "Blade";
        if (strncmp(skeleton->bones[i].name, blade_name, 5) == 0 && weapon->trace_point_count < MAX_WEAPON_BLADE_BONES)
        {
            weapon->trace_bones_idx[weapon->trace_point_count++] = i;
        }
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
    ray.start = Sol_Model_GetBoneXform(world, id, "Blade.001").pos;
    forc(world, id, ScEquip)
    {
        vec3s blade_pos = Sol_Weapon_BladeXform(world, right_hand ? c->right_weapon : c->left_weapon).pos;
        SolDistDir dd   = Sol_GetDistDir(blade_pos, ray.start);
        ray.dir         = dd.dir;
        ray.dist        = dd.dist;
        ray.mask        = COLLAYER_ALL;
        ray.radius      = 0.4f;
        ray.ignoreEnt   = id;
        return ray;
    }

    return ray;
}

int Sol_Weapon_Trace(World *world, int id, SolHit hit, u32 hitgen)
{
    int hits = 0;
    forc(world, id, ScWeapon)
    {
        for (int i = 0; i < c->trace_point_count - 1; i++)
        {
            int boneA = c->trace_bones_idx[i];
            int boneB = c->trace_bones_idx[i + 1];
            SolRay ray;
            ray.start      = Sol_Model_GetBoneByIdx(world, id, boneA);
            vec3s next_pos = Sol_Model_GetBoneByIdx(world, id, boneB);
            SolDistDir dd  = Sol_GetDistDir(next_pos, ray.start);
            ray.dir        = dd.dir;
            ray.dist       = dd.dist;
            ray.mask       = COLLAYER_ALL;
            ray.radius     = 0.4f;
            ray.ignoreEnt  = id;
            hits += Sol_Combat_DamageCast(world, id, ray, hit, hitgen);
        }
        return hits;
    }
}

void Sol_Weapon_PlayAudio(World *world, int id)
{
    ScAudio *audio = Sol_Comp_Add(world, id, ScAudio);

    // memcpy(audio->bone_follow, )
}