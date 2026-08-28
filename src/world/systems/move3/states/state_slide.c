#include "move3/s_move3.h"
#include "core/sol_core.h"
#include "world.h"
#include "sol_math.h"

#define BOOST_CD 2.5f

static bool LeaveState(World *world, int id, SolMove3 *move, SolController *cont)
{
    if (move->groundtime > 0 && Sol_Physx_GetSpeed(world, id) < 5.5f)
        if (Sol_Movement_SetState(world, id, MOVE_IDLE))
            return true;
    if (!(cont->actionState & BITC(ACTION_CROUCH)))
        if (Sol_Movement_SetState(world, id, MOVE_IDLE))
            return true;
    if (cont->actionState & BITC(ACTION_JUMP))
        if (Sol_Movement_SetState(world, id, MOVE_JUMP))
            return true;
    return false;
}

void Slide_State_Update(World *world, int id, float dt)
{
    float fdt = (float)dt;

    SolMove3   *move  = Sol_Comp_Get(world, id, SolMove3);
    SolController *cont  = Sol_Comp_Get(world, id, SolController);
    SolXform      *xform = Sol_Comp_Get(world, id, SolXform);
    if (LeaveState(world, id, move, cont))
        return;
    MoveStateData *data   = &move->stateData[move->state];
    vec3s          vel    = {0};//Sol_Physx_GetVel(world, id);
    vec3s          rot    = Sol_RotFromQuat(xform->rot);
    vec3s          latvel = vel;
    latvel.y              = 0;
    latvel                = vecNorm(latvel);
    data->as.slide.boost  = fmaxf(data->as.slide.boost - fdt, 0.0f);

    if (move->groundDot > 0.01f && move->groundDot < 0.99f)
    {
        if(Sol_Comp_Has(world, id, SolBody3))
        {
            SolBody3 *body3 = Sol_Comp_Get(world, id, SolBody3);
            body3->vel = vecAdd(body3->vel, vecSca(vecNorm(GroundSlope(WORLD_UP)), 12.0f * fdt));
        }
    }
}

void Slide_State_Enter(World *world, int id)
{
    SolMove3   *move  = Sol_Comp_Get(world, id, SolMove3);
    SolController *cont  = Sol_Comp_Get(world, id, SolController);
    if (LeaveState(world, id, move, cont))
        return;

    MoveStateData *data = &move->stateData[move->state];
    move->targetHeight  = move->baseHeight * 0.65f;
    if (move->groundtime > 0)
    {
        data->as.slide.boost = fminf(data->as.slide.boost + (solState.appTime - data->lastExited), BOOST_CD);
        // Sol_Physx_Impulse(world, id,
        //                   vecSca(vecNorm(ProjectOntoGround(world, id, Sol_Physx_GetVelDir(world, id))),
        //                          Sol_Math_MapRange(0.0f, 400.0f, 0.0f, BOOST_CD, data->as.slide.boost)));
        data->as.slide.boost /= 2.0f;
    }
}

void Slide_State_Exit(World *world, int id)
{
    SolMove3   *move  = Sol_Comp_Get(world, id, SolMove3);
    SolController *cont  = Sol_Comp_Get(world, id, SolController);
    MoveStateData *data = &move->stateData[MOVE_SLIDE];
    move->targetHeight  = move->baseHeight;
}

bool Slide_State_CanExit(World *world, int id, u32 nextState)
{
    return true;
}

bool Slide_State_CanEnter(World *world, int id, u32 lastState, u32 nextState, int slot)
{
    return true;
    // return Sol_Physx_GetSpeed(world, id) > 5.5f;
}
