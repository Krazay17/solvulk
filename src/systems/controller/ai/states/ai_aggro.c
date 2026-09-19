#include "controller/ai/s_ai.h"
#include "world.h"
#include "sol_math.h"

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
    cmd->actionState = 0;
    cmd->wishdir     = ai->dirToTarget;
    cmd->aimdir      = ai->dirToTarget;
    cmd->aimpos      = Xform_Get(world, ai->target).pos;
    vec3s pos = Xform_Get(world, id).pos;
    float dist = glms_vec3_norm(glms_vec3_sub(cmd->aimpos, pos));

    ScBody3 *target_body = Sol_Comp_Get(world, ai->target, ScBody3);
    if (target_body)
    {
        vec3s target_vel = target_body->vel;
        target_vel.y =  min(1.0f, max(-1.0f,target_vel.y));
        cmd->aimpos = vecAdd(cmd->aimpos, target_vel);
        cmd->aimpos.y += dist * 0.1f;
    }
    SolLine *line = Sol_Debug_NewLine(world, 0.1f);
    line->a = pos;
    line->b = cmd->aimpos;
    line->aColor = VEC4_GREEN;
    line->bColor = VEC4_GREEN;

    data->accum += dt;
    if (data->accum >= data->attacktimer)
    {
        data->accum -= data->attacktimer;
        data->attacktimer = Sol_Math_RandRange2(0.5f, 3.0f);
    }
    else
        cmd->actionState |= BITC(ACTION_ABILITY2);
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