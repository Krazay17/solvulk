#include "controller/ai/s_ai.h"
#include "world.h"
#include "sol_math.h"

#define PATROL_TURN_TIME 5.0f

void Ai_Patrol_Update(World *world, int id, ScAi *ai, float dt)
{
    AiStateData *data = &ai->stateData[ai->state];
    data->accum += dt;
    vec3s pos = world->xform.pos[id];

    ScCmd *cmd = Sol_Comp_Get(world, id, ScCmd);
    if (!cmd)
        return;

    cmd->wishdir = cmd->lookdir;
    if (data->accum >= PATROL_TURN_TIME)
    {
        data->accum -= PATROL_TURN_TIME;

        vec3s to_home        = vecSub(world->xform.home_pos[id], pos);
        float d2             = glms_vec3_norm2(to_home);
        float max_home_range = pow(ai->maxHomeRange + 2.0f, 2.0f);
        if (d2 > max_home_range)
        {
            float dsq    = sqrt(d2);
            cmd->lookdir = vecSca(to_home, 1.0f / dsq);
        }
        else
            cmd->lookdir = vecSca(cmd->lookdir, -1.0f);
    }
}

void Ai_Patrol_Enter(World *world, int id, ScAi *ai)
{
    ScCmd *cmd = Sol_Comp_Get(world, id, ScCmd);
    if (!cmd)
        return;
    cmd->isWalking  = true;
    cmd->isStrafing = false;
}

void Ai_Patrol_Exit(World *world, int id, ScAi *ai)
{
    ScCmd *cmd = Sol_Comp_Get(world, id, ScCmd);
    if (!cmd)
        return;
    cmd->isWalking  = false;
}

bool Ai_Patrol_CanExit(World *world, int id, ScAi *ai, u32 next)
{
    return true;
}

bool Ai_Patrol_CanEnter(World *world, int id, ScAi *ai, u32 last)
{
    return true;
}

const AiStateFuncs ai_patrol_state = {
    .update   = Ai_Patrol_Update,
    .enter    = Ai_Patrol_Enter,
    .exit     = Ai_Patrol_Enter,
    .canExit  = Ai_Patrol_CanExit,
    .canEnter = Ai_Patrol_CanEnter,
};
