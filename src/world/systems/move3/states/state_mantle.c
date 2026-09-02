/*
 * File: state_mantle.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-07-05
 *
 */

#include "move3/s_move3.h"
#include "world.h"
#include "sol_math.h"

#define RAY_COUNT 12
#define MANTLE_TIME 0.7f

static bool CheckWall(World *world, int id)
{
    ScMove3      *move  = Sol_Comp_Get(world, id, ScMove3);
    ScController *cont  = Sol_Comp_Get(world, id, ScController);
    ScBody3      *body  = Sol_Comp_Get(world, id, ScBody3);
    ScXform      *xform = Sol_Comp_Get(world, id, ScXform);

    MoveStateData *data    = &move->stateData[MOVE_MANTLE];
    vec3s          basePos = vecAdd(xform->pos, vecSca(WORLD_UP, body->dims.y * 0.7f));

    if (Sol_Raycast1(world, (SolRay){.start = xform->pos, .dir = WORLD_UP, .dist = body->dims.y, .ignoreEnt = id},
                     NULL))
        return false;

    vec3s goodPos     = {0};
    u32   mantleSpace = 0;
    // Trace top down to find ledge
    for (int i = 0; i < RAY_COUNT; i++)
    {
        float offset = (float)i * (body->dims.y / ((float)RAY_COUNT * 0.8f));
        vec3s pos    = basePos;
        pos.y -= offset;
        SolRay ray = {
            .start = pos, .dist = body->dims.x * 2.0f, .ignoreEnt = id, .dir = Sol_Vec3_FromYawPitch(cont->yaw, 0)};
        SolRayResult rayResult;
        bool         hit = Sol_Raycast1D(world, ray, &rayResult, 0.1f);
        // No hit indicates there is space above
        if (!hit)
        {
            goodPos = vecAdd(ray.start, vecSca(ray.dir, ray.dist));
            goodPos.y += body->dims.y * 0.5f;
            mantleSpace++;
        }
        // Hit after no hit indicates there is floor to mantle
        else if (mantleSpace > 2)
        {
            data->as.mantle.ledge_pos = rayResult.pos;
            data->as.mantle.pos       = goodPos;
            return true;
        }
    }
    return false;
}

static bool LeaveState(World *world, int id, ScMove3 *move, ScController *cont)
{
    if (move->stateData[move->state].elapsed >= MANTLE_TIME)
        return true;
    if (!(cont->actionState & BITC(ACTION_JUMP)))
        return true;
    if (cont->actionState & BITC(ACTION_CROUCH))
        return true;
    if (move->stateData[move->state].as.mantle.closeEnough)
        return true;

    return false;
}

void Mantle_State_Update(World *world, int id, float dt)
{
    ScMove3       *move  = Sol_Comp_Get(world, id, ScMove3);
    ScController  *cont  = Sol_Comp_Get(world, id, ScController);
    ScXform       *xform = Sol_Comp_Get(world, id, ScXform);
    ScBody3       *body  = Sol_Comp_Get(world, id, ScBody3);
    MoveStateData *data  = &move->stateData[MOVE_MANTLE];
    if (LeaveState(world, id, move, cont))
        if (Sol_Move3_SetState(world, id, MOVE_IDLE))
            return;
    vec3s pos       = xform->pos;
    vec3s targetPos = data->as.mantle.pos;
    if (pos.y < targetPos.y)
    {
        body->vel.y = 8.0f;
    }
    else
    {
        vec3s dir            = vecSub(targetPos, pos);
        float dist           = glms_vec3_norm(dir);
        data->as.mantle.dist = dist;
        if (dist <= 0.15f && !CheckWall(world, id))
            data->as.mantle.closeEnough = 1;
        dir       = vecNorm(dir);
        body->vel = vecSca(dir, 8.0f);

        SolLine *line = Sol_Line_New(world);
        line->a       = targetPos;
        line->b       = vecAdd(targetPos, vecSca(WORLD_UP, 10.0f));
        line->aColor  = VEC4_GREEN;
        line->bColor  = VEC4_GREEN;
        line->ttl     = 2.0f;

        SolLine *lineVel = Sol_Line_New(world);
        lineVel->a       = xform->pos;
        lineVel->b       = vecAdd(xform->pos, body->vel);
        lineVel->aColor  = VEC4_GREEN;
        lineVel->bColor  = VEC4_GREEN;
        lineVel->ttl     = 2.0f;
    }
}

void Mantle_State_Enter(World *world, int id)
{
    ScMove3       *move         = Sol_Comp_Get(world, id, ScMove3);
    ScBody3       *body         = Sol_Comp_Get(world, id, ScBody3);
    ScXform       *xform        = Sol_Comp_Add(world, id, ScXform);
    ScController  *cont         = Sol_Comp_Get(world, id, ScController);
    MoveStateData *data         = &move->stateData[MOVE_MANTLE];
    move->wantsJump             = false;
    data->as.mantle.closeEnough = 0;
    data->as.mantle.doRoll      = (body->vel.y > 5.0f) && (xform->pos.y < data->as.mantle.ledge_pos.y);
}

void Mantle_State_Exit(World *world, int id)
{
}

bool Mantle_State_CanExit(World *world, int id, u32 nextState)
{
    ScMove3      *move = Sol_Comp_Get(world, id, ScMove3);
    ScController *cont = Sol_Comp_Get(world, id, ScController);
    return LeaveState(world, id, move, cont);
}

bool Mantle_State_CanEnter(World *world, int id, u32 lastState, u32 nextState, int slot)
{
    if (Sol_Comp_Has(world, id, ScAbility))
    {
        ScAbility *ability = Sol_Comp_Get(world, id, ScAbility);

        if (ability->state == ABILITY_STATE_DASH || ability->state == ABILITY_STATE_SPINSLASH)
            return false;
    }
    return CheckWall(world, id);
}

void Mantle_State_Draw(World *world, int id, double dt)
{
}
