#include "sol_core.h"

#include "ability_i.h"

#define HITINTERVAL 0.1f
#define HITDELAY 0.1f

void Laser_State_Update(World *world, int id, float dt)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ability->activeSlot];
    CompCombat  *combat  = &world->combats[id];
    data->elapsed += dt;

    if (data->elapsed >= data->duration)
    {
        Sol_Ability_SetState(world, id, ABILITY_STATE_IDLE, 0, 1);
        return;
    }
    if (data->elapsed < HITDELAY)
        return;

    CompController *controller = &world->controllers[id];

    SolRay lengthCheck = {
        .dir       = controller->aimdir,
        .dist      = 15.0f,
        .ignoreEnt = id,
        .pos       = controller->aimpos,
    };
    SolRayResult lengthResult = Sol_RaycastD(world, lengthCheck, 0.5f);

    if (data->stage > 0)
        Sol_Ribbon_UpdateTargetPos(world, data->ribbonHandle, lengthResult.pos);
    if (Sol_Physx_GetVel(world, id).y < 0.0f)
        Sol_Physx_SetVelY(world, id, fmax(Sol_Physx_GetVel(world, id).y, 0.0f));

    switch (data->stage)
    {
    case 0:
        data->ribbonHandle = Sol_Ribbon_AddWithTarget(world, id, RIBBONKIND_LASER, lengthResult.pos, 1.0f,
                                                      (vec4s){1.0f, 1.0f, 1.0f, 1.0f});
        data->stage++;
        break;
    case 1:
        data->accum += dt;
        if (data->accum < HITINTERVAL)
            return;
        data->accum = 0;
        Sol_Combat_ClearHits(world, id);
        SolRay ray = {
            .dir       = controller->aimdir,
            .dist      = lengthResult.dist,
            .ignoreEnt = id,
            .pos       = controller->aimpos,
            .mask      = 0b1,
        };
        SolRayResult results[64] = {0};
        int          hits        = Sol_SphereCast(world, ray, .5f, results, 64);

        for (int i = 0; i < hits; i++)
        {
            SolRayResult result = results[i];

            float dot =
                glms_vec3_dot(controller->aimdir, glms_vec3_normalize(glms_vec3_sub(result.pos, controller->aimpos)));
            if (dot < 0)
                continue;
            if (combat->hitEnts[result.entId])
                continue;
            combat->hitEnts[result.entId] = true;
            Sol_Event_Add(world, (SolEvent){
                                     .kind              = EVENTKIND_HIT,
                                     .as.hit.damage     = data->damage,
                                     .as.hit.buffMask   = data->buffs,
                                     .as.hit.effectMask = data->effects,
                                     .as.hit.entA       = id,
                                     .as.hit.entB       = result.entId,
                                     .as.hit.pos        = result.pos,
                                     .as.hit.vel        = controller->aimdir,
                                 });
        }
        break;
    }
}

void Laser_State_Enter(World *world, int id)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ability->activeSlot];
    CompCombat  *combat  = &world->combats[id];
    data->accum          = HITINTERVAL;
    data->stage          = 0;
    Sol_Combat_ClearHits(world, id);

    float animRate           = 1.0f;
    combat->baseAnimRate     = animRate;
    combat->hitPause         = 0;
    combat->hitPauseDiminish = 0;

    AnimDesc desc = {.anim     = ability->activeSlot == 1 ? ANIM_ATTACK_RIGHT : ANIM_ATTACK_LEFT,
                     .layerId  = ANIM_LAYER_UPPER,
                     .speed    = animRate,
                     .playKind = ANIMPLAYKIND_ONESHOT,
                     .blendIn  = 0.05f};

    Sol_Model_PlayAnim(world, id, desc);
    Sol_Event_Add(world, (SolEvent){
                             .kind       = EVENTKIND_FX,
                             .as.fx.kind = FXKIND_SWORD_SWING,
                             .as.fx.pos  = Sol_Controller_GetAimPos(world, id),
                         });
}

void Laser_State_Exit(World *world, int id)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ability->activeSlot];
    data->lastExited     = Sol_GetGameTime();
    Sol_Model_PlayAnim(world, id, (AnimDesc){.layerId = ANIM_LAYER_UPPER});
    Sol_Ribbon_Kill(world, data->ribbonHandle);
}

bool Laser_State_CanExit(World *world, int id, u32 next)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ability->activeSlot];
    return data->elapsed >= data->duration * 0.8f;
}

bool Laser_State_CanEnter(World *world, int id, u32 last, u32 next, int slot)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[slot];
    return slot != ability->activeSlot && !(data->lastExited + data->cooldown > Sol_GetGameTime());
}
