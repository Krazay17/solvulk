#include "controller/ai/s_ai.h"
#include "world.h"
#include "sol_math.h"
#include "sol_core.h"

static void Ai_Aggro_Update(World *world, int id, ScAi *ai, float dt)
{
    AiStateData *data = &ai->stateData[ai->state];
    ScCmd *cmd        = Sol_Comp_Get(world, id, ScCmd);
    if (!cmd)
        return;

    int target       = ai->brain.target;
    vec3s pos        = world->xform.pos[id];
    vec3s target_pos = world->xform.pos[target];

    vec3s fwd    = ai->brain.target_dir;
    cmd->lookdir = fwd;
    cmd->leftdir = glms_vec3_cross(WORLD_UP, fwd);
    cmd->yaw     = Sol_YawFromVec(fwd);
    cmd->pitch   = Sol_PitchFromVec(fwd);
    cmd->aimpos  = target_pos;
    cmd->aimdir  = cmd->lookdir;

    ScBody3 *target_body = Sol_Comp_Get(world, target, ScBody3);
    if (target_body)
    {
        vec3s target_vel = target_body->vel;
        target_vel       = glms_vec3_clamp(target_vel, -1.5f, 1.5f);
        cmd->aimpos      = vecAdd(cmd->aimpos, target_vel);
        float mapped     = Sol_Math_Remap(ai->brain.target_dist, 1.0f, 50.0f, -target_body->dims.y, 10.0f);
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

    Fill_Reward(world, id, ai, ai->brain.target, dt);
    data->accum += dt;
    if (data->accum >= ai->learning.actionTimer)
    {
        data->accum -= ai->learning.actionTimer;
        // ai->learning.actionTimer = Sol_Math_RandRange2(0.05f, 0.2f);
        ai->learning.actionTimer = 0.1f;
        Submit_Learn(world, id, ai, cmd, ai->brain.target, (float *)solData.qtable.q, (float *)solData.qtable.qc);
    }
}

static void Ai_Aggro_Enter(World *world, int id, ScAi *ai)
{
    AiStateData *data = &ai->stateData[ai->state];
    ScCmd *cmd        = Sol_Comp_Get(world, id, ScCmd);
    if (!cmd)
        return;
    cmd->isWalking  = false;
    cmd->isStrafing = true;
}

static void Ai_Aggro_Exit(World *world, int id, ScAi *ai)
{
}

static bool Ai_Aggro_CanExit(World *world, int id, ScAi *ai, u32 next)
{
    return true;
}

static bool Ai_Aggro_CanEnter(World *world, int id, ScAi *ai, u32 last)
{
    AiStateData *data = &ai->stateData[ai->state];
    return ai->brain.target != 0;
}

static void Ai_Wizard_Aggro_Update(World *world, int id, ScAi *ai, float dt)
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