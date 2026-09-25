#include "move3/s_move3.h"
#include "world.h"
#include "sol_math.h"

#define JUMP_BUFFER 0.1f

#define JUMP_VEL 11.0f
#define JUMP_DURATION 0.5f
#define DAMPING 3.0f

void Move_Jump_Update(World *world, int id, ScMove3 *move, ScCmd *cmd, float dt)
{
    ScBody3 *body3      = Sol_Comp_Get(world, id, ScBody3);
    MoveStateData *data = &move->stateData[MOVE_JUMP];

    // if (data->elapsed >= JUMP_DURATION)
    // {
    //     Sol_Move3_SetState(world, id, MOVE_IDLE);
    //     return;
    // }
    // else if (cmd->actionState & BITC(ACTION_JUMP) && data->elapsed > JUMP_DURATION * 0.1f)
    //     if (Sol_Move3_SetState(world, id, MOVE_WALLRUN))
    //         return;

    float alpha = 1.0f - (data->elapsed / JUMP_DURATION);

    body3->vel = Sol_Math_DampDir(body3->vel, WORLD_UP, alpha, DAMPING, dt);
}

void Move_Jump_Enter(World *world, int id, ScMove3 *move, ScCmd *cmd)
{
    MoveStateData *data = &move->stateData[MOVE_JUMP];

    data->as.jump.airJump = false;
    move->wantsJump       = false;

    if (move->airtime >= JUMP_BUFFER)
    {
        ScCombat *combat = Sol_Comp_Get(world, id, ScCombat);
        if (combat)
            combat->energy -= 25.0f;
        data->as.jump.airJump = true;
    }

    move->groundtime = 0;
    move->airtime    = JUMP_BUFFER;

    ScBody3 *body3 = Sol_Comp_Get(world, id, ScBody3);
    if (!body3)
        return;
    if (body3->vel.y < 0)
        body3->vel.y = 0;
    vec3s dir  = glms_vec3_normalize(glms_vec3_lerp(move->groundNorm, WORLD_UP, 0.9f));
    body3->vel = vecAdd(body3->vel, vecSca(dir, JUMP_VEL));
}

void Move_Jump_Exit(World *world, int id, ScMove3 *move, ScCmd *cmd)
{
}

bool Move_Jump_CanExit(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 next)
{
    switch (next)
    {
    case MOVE_WALLRUN:
    case MOVE_WALLJUMP:
        return true;
        break;
    }
    return move->stateData[move->state].elapsed > JUMP_DURATION;
}

bool Move_Jump_CanEnter(World *world, int id, ScMove3 *move, ScCmd *cmd, u32 last)
{
    if (move->wantsJump)
    {
        if (move->airtime >= JUMP_BUFFER)
        {
            if (Sol_Comp_Has(world, id, ScCombat))
            {
                ScCombat *combat = Sol_Comp_Get(world, id, ScCombat);
                return combat->energy >= 25.0f;
            }
            return false;
        }
        return true;
    }
    return false;
}

const MoveStateFuncs move_jump_funcs = {
    .update   = Move_Jump_Update,
    .enter    = Move_Jump_Enter,
    .exit     = Move_Jump_Exit,
    .canExit  = Move_Jump_CanExit,
    .canEnter = Move_Jump_CanEnter,
};
