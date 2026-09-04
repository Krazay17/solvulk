#include "move3/s_move3.h"
#include "core/sol_core.h"
#include "world.h"
#include "sol_math.h"

#define BOOST_CD 2.5f

static bool LeaveState(World *world, int id, ScMove3 *move, ScCmd *cmd)
{
    if (move->groundtime > 0 && Sol_Body3_GetSpeed(world, id) < 5.5f)
        if (Sol_Move3_SetState(world, id, MOVE_IDLE))
            return true;
    if (!(cmd->actionState & BITC(ACTION_CROUCH)))
        if (Sol_Move3_SetState(world, id, MOVE_IDLE))
            return true;
    if (cmd->actionState & BITC(ACTION_JUMP))
        if (Sol_Move3_SetState(world, id, MOVE_JUMP))
            return true;
    return false;
}

void Slide_State_Update(World *world, int id, float dt)
{
    float fdt = (float)dt;

    ScMove3      *move = Sol_Comp_Get(world, id, ScMove3);
    ScCmd *cmd = Sol_Comp_Get(world, id, ScCmd);
    if (LeaveState(world, id, move, cmd))
        return;

    ScXform       *xform  = Sol_Comp_Get(world, id, ScXform);
    ScBody3       *body   = Sol_Comp_Get(world, id, ScBody3);
    MoveStateData *data   = &move->stateData[move->state];
    vec3s          vel    = body->vel;
    vec3s          rot    = Sol_RotFromQuat(xform->rot);
    vec3s          latvel = vel;
    latvel.y              = 0;
    latvel                = vecNorm(latvel);
    data->as.slide.boost  = fmaxf(data->as.slide.boost - fdt, 0.0f);

    if (move->groundDot > 0.01f && move->groundDot < 0.99f)
    {
        body->vel = vecAdd(body->vel, vecSca(vecNorm(GroundSlope(move->groundNorm)), 12.0f * fdt));
    }
}

void Slide_State_Enter(World *world, int id)
{
    ScMove3      *move = Sol_Comp_Get(world, id, ScMove3);
    ScCmd *cmd = Sol_Comp_Get(world, id, ScCmd);
    if (LeaveState(world, id, move, cmd))
        return;

    ScBody3       *body = Sol_Comp_Get(world, id, ScBody3);
    MoveStateData *data = &move->stateData[move->state];

    move->targetHeight = move->baseHeight * 0.65f;
    if (move->groundtime > 0)
    {
        data->as.slide.boost = fminf(data->as.slide.boost + (solState.appTime - data->lastExited), BOOST_CD);
        body->impulse        = vecSca(vecNorm(ProjectOntoGround(move->groundNorm, Sol_Body3_GetDir(world, id))),
                                      Sol_Math_MapRange(0.0f, 400.0f, 0.0f, BOOST_CD, data->as.slide.boost));

        data->as.slide.boost /= 2.0f;
    }
}

void Slide_State_Exit(World *world, int id)
{
    ScMove3       *move = Sol_Comp_Get(world, id, ScMove3);
    ScCmd  *cmd = Sol_Comp_Get(world, id, ScCmd);
    MoveStateData *data = &move->stateData[MOVE_SLIDE];
    move->targetHeight  = move->baseHeight;
}

bool Slide_State_CanExit(World *world, int id, u32 nextState)
{
    return true;
}

bool Slide_State_CanEnter(World *world, int id, u32 lastState, u32 nextState, int slot)
{
    return Sol_Body3_GetSpeed(world, id) > 5.5f;
}
