#include "controller/ai/s_ai.h"
#include "world.h"

void Ai_Aggro_Update(World *world, int id, ScAi *ai, float dt)
{
    AiStateData *data = &ai->stateData[ai->state];
}

void Ai_Aggro_Enter(World *world, int id, ScAi *ai)
{
    AiStateData *data = &ai->stateData[ai->state];
}

void Ai_Aggro_Exit(World *world, int id, ScAi *ai)
{
}

bool Ai_Aggro_CanExit(World *world, int id, ScAi *ai, u32 next)
{
    return true;
}

bool Ai_Aggro_CanEnter(World *world, int id, ScAi *ai, u32 last)
{
    AiStateData *data = &ai->stateData[ai->state];
    return ai->target;
}

void Ai_Wizard_Aggro_Update(World *world, int id, ScAi *ai, float dt)
{
    AiStateData *data = &ai->stateData[ai->state];
    ScCmd *cmd        = Sol_Comp_Get(world, id, ScCmd);
    if (!cmd)
        return;
    cmd->wishdir = ai->dirToTarget;
    cmd->aimdir  = ai->dirToTarget;
    cmd->aimpos  = Xform_Get(world, ai->target).pos;
    cmd->actionState |= BITC(ACTION_ABILITY2);
    // bool abil    = Sol_Ability_SetState(world, id, ABILITY_STATE_FIREBALL, 1, false);
}

const AiStateFuncs ai_aggro_state = {
    .update   = Ai_Aggro_Update,
    .enter    = Ai_Aggro_Enter,
    .exit     = Ai_Aggro_Enter,
    .canExit  = Ai_Aggro_CanExit,
    .canEnter = Ai_Aggro_CanEnter,
};

const AiStateFuncs wizard_aggro_state = {
    .update = Ai_Wizard_Aggro_Update,
};