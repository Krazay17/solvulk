#include "s_ai.h"
#include "controller/s_controller.h"
#include "sol_math.h"
#include "world.h"
#include "xform/s_xform.h"
#include "vital/s_vital.h"

void Aggro_State_Update(World *world, int id, float dt)
{
    CompController *controller   = &world->controllers[id];
    CompAi         *aicontroller = &world->aicontrollers[id];
    u32             newTarget    = AiController_FindTarget(world, id);
    if (newTarget > 0)
    {
        aicontroller->target = newTarget;
    }
    if (aicontroller->target == 0)
    {
        Ai_SetState(world, id, AISTATE_IDLE, 0);
        return;
    }
    if (Sol_Vital_GetDead(world, aicontroller->target))
    {
        Ai_SetState(world, id, AISTATE_IDLE, 0);
        return;
    }
    if (aicontroller->distToTarget > 50.0f)
    {
        aicontroller->dropAggroTimer += dt;
        if (aicontroller->dropAggroTimer > 6.0f)
        {
            aicontroller->target = 0;
        }
    }
    else
    {
        aicontroller->dropAggroTimer = 0;
    }
    AiStateData *data   = &aicontroller->stateData[aicontroller->state];
    vec3s        dir    = aicontroller->dirToTarget;
    controller->lookdir = dir;


    // Aim up a bit for lob projectile
    controller->aimdir = glms_vec3_lerp(controller->aimdir, WORLD_UP, aicontroller->distToTarget / 200.f);

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
    }
}

void Aggro_State_Enter(World *world, int id)
{

    CompAi *aicontroller                                     = &world->aicontrollers[id];
    aicontroller->dropAggroTimer                             = 0;
    aicontroller->stateData[aicontroller->state].attacktimer = 5.0f;
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
