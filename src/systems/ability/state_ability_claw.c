#include "sol_core.h"

#include "ability_i.h"

#define HITINTERVAL 0.05f
#define HITDELAY 0.15f
void Claw_State_Update(World *world, int id, float dt)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ability->activeSlot];
    data->elapsed += dt;
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
            .dist      = 3.0f,
            .ignoreEnt = id,
            .pos       = controller->aimpos,
            .mask      = 0b1,
        };
        SolRayResult results[32] = {0};
        int          hits        = Sol_SphereCast(world, ray, .66f, results, 32);
        for (int i = 0; i < hits; i++)
        {
            SolRayResult result = results[i];

            float dot =
                glms_vec3_dot(controller->aimdir, glms_vec3_normalize(glms_vec3_sub(result.pos, controller->aimpos)));
            if (dot < 0)
                continue;
            if (world->combats[id].hitEnts[result.entId])
                return;
            world->combats[id].hitEnts[result.entId] = true;
            Sol_Event_Add(world, (SolEvent){
                                     .kind              = EVENTKIND_HIT,
                                     .as.hit.damage     = data->damage,
                                     .as.hit.buffMask   = data->buffs,
                                     .as.hit.effectMask = data->effects,
                                     .as.hit.entA       = id,
                                     .as.hit.entB       = result.entId,
                                     .as.hit.pos        = result.pos,
                                     .as.hit.vel        = controller->aimdir,
                                     .as.hit.fxKind     = FXKIND_SWORD_HIT,
                                 });
            Sol_Physx_SetVelY(world, id, fmax(Sol_Physx_GetVel(world, id).y, 1.0f));
        }
    }
}

void Claw_State_Enter(World *world, int id)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ability->activeSlot];
    data->accum          = HITINTERVAL;
    Sol_Combat_ClearHits(world, id);

    // data->stage = !data->stage;
    // sollog(data->stage);
    AnimDesc desc = {.anim    = ability->activeSlot == 1 ? ANIM_ATTACK_RIGHT : ANIM_ATTACK_LEFT,
                     .layerId = ANIM_LAYER_UPPER,
                     .seek    = 0.05f,
                     .speed   = 1.0f,
                     .oneShot = true,
                     .blendIn = 0.05f};
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
    data->lastExited     = Sol_GetGameTime();
}

bool Claw_State_CanExit(World *world, int id, u32 next)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ability->activeSlot];
    return data->elapsed >= data->duration * 0.8f;
}

bool Claw_State_CanEnter(World *world, int id, u32 last, u32 next, int slot)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[slot];
    return slot != ability->activeSlot && !(data->lastExited + data->cooldown > Sol_GetGameTime());
}
