#include "move3/s_move3.h"
#include "world.h"
#include "sol_core.h"
#include "sol_math.h"

#define BOOST_CD 2.5f

void Move_Slide_Update(World *world, int id, ScMove3 *move, ScCmd *cmd, float dt)
{
    float fdt = (float)dt;

    ScXform       *xform = Sol_Comp_Get(world, id, ScXform);
    ScBody3       *body  = Sol_Comp_Get(world, id, ScBody3);
    MoveStateData *data  = &move->stateData[move->state];
    vec3s          vel   = body->vel;
    data->vel            = vel;
    vec3s rot            = Sol_RotFromQuat(xform->rot);
    vec3s latvel         = vel;
    latvel.y             = 0;
    latvel               = vecNorm(latvel);
    data->as.slide.boost = fmaxf(data->as.slide.boost - fdt, 0.0f);

    if (move->groundNorm.y > 0.01f && move->groundNorm.y < 0.99f)
    {
        body->vel = vecAdd(body->vel, vecSca(vecNorm(GroundSlope(move->groundNorm)), 12.0f * fdt));
    }
}

void Move_Slide_Enter(World *world, int id, ScMove3 *move, ScCmd *cmd)
{
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

void Move_Slide_Exit(World *world, int id, ScMove3 *move, ScCmd *cmd)
{
    MoveStateData *data = &move->stateData[MOVE_SLIDE];
    move->targetHeight  = move->baseHeight;
}

bool Move_Slide_CanExit(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 next)
{
    return true;
}

bool Move_Slide_CanEnter(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 last)
{
    if (cmd->actionState & BITC(ACTION_CROUCH) && Sol_Body3_GetSpeed(world, id) > 5.5f)
        return true;
    return false;
}
