#include "sol_core.h"

#include "ability_i.h"

#define DASH_VEL 20.0f
#define DASH_ALPHAMOD 1.5f
#define HITINTERVAL 0.05f
#define HITRADIUS 1.7f

void ADash_State_Update(World *world, int id, float dt)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ability->activeSlot];
    float       *elapsed = &data->elapsed;
    data->accum += dt;
    *elapsed += dt;
    if (*elapsed >= data->duration)
    {
        Sol_Ability_SetState(world, id, ABILITY_STATE_IDLE, 0, false);
        return;
    }
    float alpha = DASH_ALPHAMOD - (*elapsed / data->duration);

    Sol_Physx_SetVel(world, id, glms_vec3_scale(data->enterDir, alpha * DASH_VEL));

    if (!(data->effects & EFFECTMASK_KNOCKBACK))
        return;
    if (data->accum > HITINTERVAL)
    {
        data->accum               = 0;
        vec3s pos                 = Sol_Xform_GetPos(world, id);
        pos                       = glms_vec3_add(pos, glms_vec3_scale(Sol_Physx_GetVelDir(world, id), 1.0f));
        SolRayResult results[256] = {0};
        int          hits         = Sol_SphereCast(world, (SolRay){.pos = pos, .ignoreEnt = id}, HITRADIUS, results, 256);
        for (int i = 0; i < hits; i++)
        {
            CompCombat *combat = &world->combats[id];
            if (!Sol_Owner_GetHostile(world, id, results[i].entId))
                continue;
            if (combat->hitEnts[results[i].entId])
                continue;
            combat->hitEnts[results[i].entId] = true;
            
            Sol_Event_Add(world, (SolEvent){
                                     .kind              = EVENTKIND_HIT,
                                     .as.hit.entA       = id,
                                     .as.hit.entB       = results[i].entId,
                                     .as.hit.pos        = results[i].pos,
                                     .as.hit.damage     = data->damage,
                                     .as.hit.effectMask = data->effects,
                                     .as.hit.buffMask   = data->buffs,
                                     .as.hit.kind       = HITKIND_SHIELD_PULSE,
                                     .as.hit.vel        = vecSub(results[i].pos, pos),
                                     .as.hit.fxKind     = FXKIND_SPINHIT,
                                 });
        }
    }
}

void ADash_State_Enter(World *world, int id)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ability->activeSlot];
    Sol_Audio_PlayAt(SOL_AUDIO_DASH, Sol_Controller_GetAimPos(world, id), 1.0f, 0, 0);
    Buff *buff = Sol_Buff_Add(world, id, BUFFKIND_INVULN);
    if (buff)
        buff->duration = data->duration * 0.5f;
    Sol_Combat_ClearHits(world, id);

    data->enterDir = Sol_Vec3_FromYawPitch(Sol_GetYaw(world, id), 0);
    if (glms_vec3_norm(Sol_GetWishdir(world, id)) > 0 && vecDot(Sol_GetWishdir(world, id), WORLD_UP) < 0.99f)
        data->enterDir = Sol_GetWishdir(world, id);
    data->enterDir.y = 0;
    data->enterDir   = glms_vec3_normalize(data->enterDir);

    vec3s    rot  = Sol_RotFromQuat(world->xforms[id].quat);
    AnimDesc desc = {.layerId = ANIM_LAYER_OVERRIDE,
                     .oneShot = true,
                     .seek    = 0.05f,
                     .speed   = 1.6f - data->duration,
                     .blendIn = 0.05f};

    switch (Sol_GetStrafedir(data->enterDir.x, data->enterDir.z, rot.x, rot.z))
    {
    case STRAFE_FWD:
    case STRAFE_FWD_LEFT:
    case STRAFE_FWD_RIGHT:
        desc.anim = ANIM_DASH_FWD;
        break;
    case STRAFE_BWD:
    case STRAFE_BWD_LEFT:
    case STRAFE_BWD_RIGHT:
        desc.anim = ANIM_DASH_BWD;
        break;
    case STRAFE_LEFT:
        desc.anim = ANIM_DASH_LEFT;
        break;
    case STRAFE_RIGHT:
        desc.anim = ANIM_DASH_RIGHT;
        break;
    default:
        desc.anim = ANIM_DASH_FWD;
        break;
    }
    Sol_Model_PlayAnim(world, id, desc);
}

void ADash_State_Exit(World *world, int id)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ability->activeSlot];
    data->lastExited     = Sol_GetGameTime();
    Sol_Model_PlayAnim(world, id, (AnimDesc){.layerId = ANIM_LAYER_OVERRIDE});
}

bool ADash_State_CanExit(World *world, int id, u32 next)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ability->activeSlot];
    return data->elapsed >= data->duration * 0.9f;
}

bool ADash_State_CanEnter(World *world, int id, u32 last, u32 next, int slot)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[slot];
    return slot != ability->activeSlot && !(data->lastExited + data->cooldown > Sol_GetGameTime());
}
