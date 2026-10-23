#include "world.h"
#include "controller/ai/s_ai.h"
#include "sol_math.h"
#include "sol_core.h"

static void Update(World *world, int id, ScAi *ai, float dt)
{
    ScCmd *cmd = Sol_Comp_Get(world, id, ScCmd);

    int team                     = -1;
    int crystal                  = -1;
    forc(world, id, ScTeam) team = c->team;
    forc(world, 0, SlRules)
    {
        crystal          = team == 0 ? c->as.crystal_push.crystal_b : c->as.crystal_push.crystal_a;
        vec3s target_pos = world->xform.pos[crystal];
        vec3s delta      = vecSub(target_pos, world->xform.pos[id]);
        float d2         = vecDot(delta, delta);
        float dist       = sqrt(d2);
        vec3s fwd        = vecSca(delta, 1.0f / dist);

        ai->brain.goal_pos = target_pos;
        cmd->lookdir       = fwd;
        cmd->leftdir       = glms_vec3_cross(WORLD_UP, fwd);
        cmd->yaw           = Sol_YawFromVec(fwd);
        cmd->pitch         = Sol_PitchFromVec(fwd);
        cmd->aimpos        = target_pos;
        cmd->aimdir        = cmd->lookdir;
    }
    AiStateData *data = &ai->stateData[ai->state];

    Fill_Reward(world, id, ai, crystal, dt);
    data->accum += dt;
    if (data->accum >= ai->learning.actionTimer)
    {
        data->accum -= ai->learning.actionTimer;
        ai->learning.actionTimer = Sol_Math_RandRange2(0.1f, 0.5f);
        Submit_Learn(world, id, ai, cmd, crystal, (float *)solData.qtable.q_path, NULL);
    }
}
static void Enter(World *world, int id, ScAi *ai)
{
    Sol_Cmd_Clear(world, id);
}
static void Exit(World *world, int id, ScAi *ai)
{
}
static bool CanEnter(World *world, int id, ScAi *ai, u32 next)
{
    return true;
}
static bool CanExit(World *world, int id, ScAi *ai, u32 last)
{
    return true;
}
const AiStateFuncs ai_crystal_push = {
    Update, Enter, Exit, CanExit, CanEnter,
};