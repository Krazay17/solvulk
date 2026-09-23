/*
 * File: s_ai.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-17
 *
 */
#include "s_ai.h"
#include "world.h"
#include "sol_math.h"
#include "sol_core.h"

const u32 AI_STATE_PRIORITY[AISTATE_COUNT] = {
    AISTATE_AGGRO, //
    // AISTATE_RETREAT, //
    // AISTATE_SEARCH,  //
    // AISTATE_PATROL,  //
    AISTATE_IDLE, //
};

extern const AiStateFuncs ai_idle_state;
extern const AiStateFuncs ai_patrol_state;
extern const AiStateFuncs ai_aggro_state;
// extern const AiStateFuncs search_state;
// extern const AiStateFuncs retreat_state;
const AiStateFuncs *ai_funcs_base[AISTATE_COUNT] = {
    [AISTATE_IDLE]   = &ai_idle_state,   //
    [AISTATE_PATROL] = &ai_patrol_state, //
    [AISTATE_AGGRO]  = &ai_aggro_state,  //
    // [AISTATE_SEARCH]  = &search_state,  //
    // [AISTATE_RETREAT] = &retreat_state, //
};

extern const AiStateFuncs wizard_aggro_state;
const AiStateFuncs *ai_kind_funcs[AIKIND_COUNT][AISTATE_COUNT] = {
    [AIKIND_WIZARD][AISTATE_AGGRO] = &wizard_aggro_state,
};

static inline AiStateFuncs Ai_Get_Funcs(AiKind kind, AiState state)
{
    const AiStateFuncs *override = ai_kind_funcs[kind][state];
    const AiStateFuncs *base     = ai_funcs_base[state];

    AiStateFuncs f = base ? *base : (AiStateFuncs){0};

    if (override)
    {
        if (override->update)
            f.update = override->update;
        if (override->enter)
            f.enter = override->enter;
        if (override->exit)
            f.exit = override->exit;
        if (override->canEnter)
            f.canEnter = override->canEnter;
        if (override->canExit)
            f.canExit = override->canExit;
    }

    return f;
}

static void Evaluate_State(World *world, int id, ScAi *ai)
{
    u32 current_state                      = ai->state;
    const AiStateFuncs current_state_funcs = Ai_Get_Funcs(ai->kind, current_state);

    for (int i = 0; i < AISTATE_COUNT; i++)
    {
        u32 target_state                      = AI_STATE_PRIORITY[i];
        const AiStateFuncs target_state_funcs = Ai_Get_Funcs(ai->kind, target_state);

        if (current_state_funcs.canExit && !current_state_funcs.canExit(world, id, ai, target_state))
            continue;
        if (target_state_funcs.canEnter && !target_state_funcs.canEnter(world, id, ai, current_state))
            continue;
        if (current_state == target_state)
            break;
        // sollog(current_state_funcs.canExit(world, id, ai, target_state),
        //        target_state_funcs.canEnter(world, id, ai, current_state), i);

        if (current_state_funcs.exit)
            current_state_funcs.exit(world, id, ai);
        ai->state = target_state;
        if (target_state_funcs.enter)
            target_state_funcs.enter(world, id, ai);
        break;
    }
}

void Ai_Step(World *world, double dt)
{
    float fdt = (float)dt;

    SparseSet_ScAi *set = Sol_Comp_Set(world, ScAi);

    int count = set->cnt;
    for (int i = 0; i < count; i++)
    {
        int id      = set->dense[i];
        ScAi *ai    = &set->data[i];
        ScCmd *cmd  = Sol_Comp_Get(world, id, ScCmd);
        Xform xform = Xform_Get(world, id);
        if (!cmd)
            continue;
        Fill_Brain(world, id, ai, cmd, fdt);
        if (ai->brain.target)
        {
            vec3s fwd    = ai->brain.target_dir;
            cmd->lookdir = fwd;
            cmd->leftdir = glms_vec3_cross(WORLD_UP, fwd);
            cmd->yaw     = Sol_YawFromVec(fwd);
        }
        Evaluate_State(world, id, ai);
        AiStateFuncs funcs = Ai_Get_Funcs(ai->kind, ai->state);
        if (funcs.update)
            funcs.update(world, id, ai, fdt);
    }
}

void Sol_Ai_QuickLearn(World *world, int id, int ownerId, bool once)
{
    ScAilearn *ailearn = Sol_Comp_Get(world, id, ScAilearn);
    ScAi *ai           = Sol_Comp_Get(world, ownerId, ScAi);
    if (!ailearn || !ai)
        return;
    ScCombat *owner_combat = Sol_Comp_Get(world, ownerId, ScCombat);
    bool owner_alive       = owner_combat ? !owner_combat->is_dead : false;
    if (owner_alive)
    {
        float *q = &solData.qtable.q[ailearn->knows.raw][ailearn->action];
        *q += (10.0f - *q);
    }
    if (once)
    {
        Sol_Comp_Rem(world, id, ScAilearn);
    }
}