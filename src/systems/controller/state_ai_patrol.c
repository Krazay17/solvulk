#include "sol_core.h"

#include "controller_i.h"

void Patrol_State_Update(World *world, int id, float dt)
{
    
}

void Patrol_State_Enter(World *world, int id)
{

}

void Patrol_State_Exit(World *world, int id)
{

}

bool Patrol_State_CanExit(World *world, int id)
{
    return true;
}

bool Patrol_State_CanEnter(World *world, int id)
{
    return true;
}
AiStateData *Ai_GetStatedata(World *world, int id)
{
    CompAiController *aicontroller = &world->aicontrollers[id];
    return &aicontroller->stateData[aicontroller->state];
}