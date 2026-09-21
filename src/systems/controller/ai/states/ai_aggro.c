#include "controller/ai/s_ai.h"
#include "world.h"
#include "sol_math.h"
#include "sol_core.h"

void Ai_Aggro_Update(World *world, int id, ScAi *ai, float dt)
{
    AiStateData *data = &ai->stateData[ai->state];
    int target        = ai->brain.target;
    vec3s pos         = world->xform.pos[id];
    vec3s target_pos  = world->xform.pos[target];

    ScCmd *cmd = Sol_Comp_Get(world, id, ScCmd);

    cmd->aimpos = target_pos;

    ScBody3 *target_body = Sol_Comp_Get(world, target, ScBody3);
    if (target_body)
    {
        vec3s target_vel = target_body->vel;
        target_vel       = glms_vec3_clamp(target_vel, -5.0f, 5.0f);
        cmd->aimpos      = vecAdd(cmd->aimpos, target_vel);
        float mapped     = Sol_Math_MapRange(-1.0f, 10.0f, 1.0f, 50.0f, ai->brain.target_dist);
        cmd->aimpos.y += mapped;
    }

    if (solState.debug)
    {
        SolLine *line = Sol_Debug_NewLine(world, 0.1f);
        line->a       = pos;
        line->b       = cmd->aimpos;
        line->aColor  = VEC4_GREEN;
        line->bColor  = VEC4_GREEN;
    }

    Fill_Reward(world, id, ai, dt);
    data->accum += dt;
    if (data->accum >= data->attacktimer)
    {
        data->accum -= data->attacktimer;
        data->attacktimer = Sol_Math_RandRange2(0.1f, 0.3f);
        // Sol_Debug_Add("AiReward", ai->reward);
        Submit_Learn(world, id, ai, cmd);
        Convert_AiActions(ai, cmd);
    }
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
    return ai->brain.target;
}

void Ai_Wizard_Aggro_Update(World *world, int id, ScAi *ai, float dt)
{
    Ai_Aggro_Update(world, id, ai, dt);
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