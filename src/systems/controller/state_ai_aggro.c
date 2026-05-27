#include "sol_core.h"

void Aggro_State_Update(World *world, int id, float dt)
{
    CompController   *controller   = &world->controllers[id];
    CompAiController *aicontroller = &world->aicontrollers[id];
    if (aicontroller->target == 0)
    {
        Ai_SetState(world, id, AISTATE_IDLE);
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
    controller->aimdir  = glms_vec3_lerp(controller->aimdir, WORLD_UP, aicontroller->distToTarget / 300.f);

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

    CompAiController *aicontroller          = &world->aicontrollers[id];
    aicontroller->dropAggroTimer            = 0;
    Ai_GetStatedata(world, id)->attacktimer = 2.0f + ((float)rand() / (float)RAND_MAX) * (8.0f - 2.0f);
}

void Aggro_State_Exit(World *world, int id)
{
}

bool Aggro_State_CanExit(World *world, int id)
{
    return true;
}

bool Aggro_State_CanEnter(World *world, int id)
{
    return true;
}
