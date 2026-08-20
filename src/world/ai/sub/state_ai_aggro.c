#include "ai/si_ai.h"

#include "sol_math.h"
#include "world.h"
#include "xform/s_xform.h"
#include "combat/s_combat.h"
#include "controller/s_controller.h"

void Aggro_State_Update(World *world, int id, float dt)
{
    CompController *controller = Sol_Controller_Get(world, id);
    CompAi         *ai         = Sol_Ai_Get(world, id);
    u32             newTarget  = Sol_Ai_FindTarget(world, id);
    if (newTarget > 0)
    {
        ai->target = newTarget;
    }
    if (ai->target == 0)
    {
        Sol_Ai_SetState(world, id, AISTATE_IDLE, 0);
        return;
    }
    if (Sol_Combat_GetDead(world, ai->target))
    {
        Sol_Ai_SetState(world, id, AISTATE_IDLE, 0);
        return;
    }
    if (ai->distToTarget > 50.0f)
    {
        ai->dropAggroTimer += dt;
        if (ai->dropAggroTimer > 6.0f)
        {
            ai->target = 0;
        }
    }
    else
    {
        ai->dropAggroTimer = 0;
    }
    AiStateData *data   = &ai->stateData[ai->state];
    vec3s        dir    = ai->dirToTarget;
    controller->lookdir = dir;

    // Aim up a bit for lob projectile
    controller->aimdir = glms_vec3_lerp(controller->aimdir, WORLD_UP, ai->distToTarget / 200.f);

    data->accum += dt;
    if (data->accum > data->attacktimer)
    {
        data->accum = 0;
        controller->actionState |= ACTION_ABILITY1;
    }
    else
    {
        controller->wishdir = dir;
        controller->actionState &= ~ACTION_ABILITY1;
        controller->actionState |= ACTION_FWD;
    }
}

void Aggro_State_Enter(World *world, int id)
{

    CompAi *ai                           = Sol_Ai_Get(world, id);
    ai->dropAggroTimer                   = 0;
    ai->stateData[ai->state].attacktimer = 5.0f;
}

void Aggro_State_Exit(World *world, int id)
{
}

bool Aggro_State_CanExit(World *world, int id, u32 next)
{
    return true;
}

bool Aggro_State_CanEnter(World *world, int id, u32 last, u32 next, int slot)
{
    return true;
}
