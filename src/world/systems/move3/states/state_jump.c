#include "move3/s_move3.h"
#include "world.h"
#include "sol_math.h"

#define JUMP_BUFFER 0.1f

#define JUMP_VEL 11.0f
#define JUMP_DURATION 0.5f
#define DAMPING 3.0f

void Sol_Movement_Jump_Update(World *world, int id, float dt)
{
    SolMove3      *move  = Sol_Comp_Get(world, id, SolMove3);
    SolController *cont  = Sol_Comp_Get(world, id, SolController);
    SolBody3      *body3 = Sol_Comp_Get(world, id, SolBody3);
    MoveStateData *data  = &move->stateData[MOVE_JUMP];

    if (data->elapsed >= JUMP_DURATION)
    {
        Sol_Movement_SetState(world, id, MOVE_IDLE);
        return;
    }
    else if (cont->actionState & BITC(ACTION_JUMP) && data->elapsed > JUMP_DURATION * 0.1f)
        if (Sol_Movement_SetState(world, id, MOVE_WALLRUN))
            return;

    float alpha = 1.0f - (data->elapsed / JUMP_DURATION);

    body3->vel = Sol_Math_DampDir(body3->vel, WORLD_UP, alpha, DAMPING, dt);
}

void Sol_Movement_Jump_Enter(World *world, int id)
{
    SolMove3      *move  = Sol_Comp_Get(world, id, SolMove3);
    SolController *cont  = Sol_Comp_Get(world, id, SolController);
    SolBody3      *body3 = Sol_Comp_Get(world, id, SolBody3);
    MoveStateData *data  = &move->stateData[MOVE_JUMP];
    move->wantsJump      = false;
    move->groundtime     = 0;
    move->airtime        = JUMP_BUFFER;

    if (body3->vel.y < 0)
        body3->vel.y = 0;
    body3->vel = vecAdd(body3->vel, vecSca(WORLD_UP, JUMP_VEL));
    // if (Sol_Physx_GetVel(world, id).y < 0)
    //     Sol_Physx_SetVelY(world, id, 0);
    // vec3s dir = glms_vec3_normalize(glms_vec3_lerp(Sol_Physx_GetGround(world, id), WORLD_UP, 0.9f));
    // Sol_Physx_AddVel(world, id, vecSca(dir, JUMP_VEL));
}

void Sol_Movement_Jump_Exit(World *world, int id)
{
}

bool Sol_Movement_Jump_CanExit(World *world, int id, u32 next)
{
    return next != MOVE_JUMP;
}

bool Sol_Movement_Jump_CanEnter(World *world, int id, u32 last, u32 next, int slot)
{
    SolMove3      *move = Sol_Comp_Get(world, id, SolMove3);
    SolController *cont = Sol_Comp_Get(world, id, SolController);
    MoveStateData *data = &move->stateData[MOVE_JUMP];
    if (!move->wantsJump || move->state == MOVE_JUMP)
        return false;
    // if (Sol_Ability_Get(world, id)->state == ABILITY_STATE_DASH)
    //     return false;

    if (move->airtime >= JUMP_BUFFER)
    {
        if (Sol_Comp_Has(world, id, SolCombat))
        {
            SolCombat *combat = Sol_Comp_Get(world, id, SolCombat);
            if (combat->energy < 25.0f)
                return false;
            else
            {
                combat->energy -= 25.0f;
                data->as.jump.airJump = true;
                return true;
            }
        }
        return false;
    }
    data->as.jump.airJump = false;
    return true;
}
