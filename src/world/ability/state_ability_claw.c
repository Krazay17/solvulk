#include "ability_i.h"
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


#define HITINTERVAL 0.05f
#define HITDELAY 0.275f
#define MELEE_RANGE 3.0f

void Claw_State_Update(World *world, int id, float dt)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ability->activeSlot];
    CompCombat  *combat  = &world->combats[id];

    combat->hitPause = fmaxf(0, combat->hitPause - dt * 5.0f);
    if (combat->hitPause > 0)
        Sol_Model_SetAnimSpeed(world, id, ANIM_LAYER_UPPER, -0.001f);
    else
    {
        Sol_Model_SetAnimSpeed(world, id, ANIM_LAYER_UPPER, combat->baseAnimRate);
        data->elapsed += dt;
    }

    if (data->elapsed >= data->duration)
    {
        Sol_Ability_SetState(world, id, ABILITY_STATE_IDLE, 0, 1);
        return;
    }
    if (data->elapsed < HITDELAY)
        return;
    data->accum += dt;

    if (data->accum > HITINTERVAL)
    {
        CompController *controller = &world->controllers[id];
        data->accum                = 0;

        SolRay ray = {
            .dir       = controller->aimdir,
            .dist      = MELEE_RANGE,
            .ignoreEnt = id,
            .pos       = controller->aimpos,
            .mask      = 0b1,
        };
        SolRayResult results[128];
        int          hits        = Sol_SphereCast(world, ray, .66f, results, 128);

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
                                     .as.hit.damageFx   = FXKIND_SWORD_HIT,
                                 });
            Sol_Physx_SetVelY(world, id, fmax(Sol_Physx_GetVel(world, id).y, 1.0f));
            if (combat->hitPauseDiminish < 4)
            {
                combat->hitPause = 1.0f;
                combat->hitPauseDiminish++;
            }
        }
    }
}

void Claw_State_Enter(World *world, int id)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ability->activeSlot];
    CompCombat  *combat  = &world->combats[id];
    data->accum          = HITINTERVAL;
    Sol_Combat_ClearHits(world, id);
    float animRate           = 1.0f;
    combat->baseAnimRate     = animRate;
    combat->hitPause         = 0;
    combat->hitPauseDiminish = 0;
    AnimDesc desc            = {.anim     = ability->activeSlot == 1 ? ANIM_ATTACK_RIGHT : ANIM_ATTACK_LEFT,
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

void Claw_State_Exit(World *world, int id)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ability->activeSlot];
    data->lastExited     = solState.gameTime;
    Sol_Model_PlayAnim(world, id, (AnimDesc){.layerId = ANIM_LAYER_UPPER});
}

bool Claw_State_CanExit(World *world, int id, u32 next)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ability->activeSlot];
    return data->elapsed >= data->duration * 0.5f;
}

bool Claw_State_CanEnter(World *world, int id, u32 last, u32 next, int slot)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[slot];
    return slot != ability->activeSlot && !(data->lastExited + data->cooldown > solState.gameTime);
}
