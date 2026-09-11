#include "s_ai.h"
#include "world.h"
#include "sol_math.h"

#include "sol_core.h"

const AiStateFuncs ai_funcs_base[AISTATE_COUNT] = {
    [AISTATE_IDLE] =
        {
            Ai_Idle_Update,
            Ai_Idle_Enter,
            Ai_Idle_Exit,
            Ai_Idle_CanExit,
            Ai_Idle_CanEnter,
        },
    [AISTATE_PATROL] =
        {
            Ai_Patrol_Update,
            Ai_Patrol_Enter,
            Ai_Patrol_Exit,
            Ai_Patrol_CanExit,
            Ai_Patrol_CanEnter,
        },
    [AISTATE_AGGRO] =
        {
            Ai_Aggro_Update,
            Ai_Aggro_Enter,
            Ai_Aggro_Exit,
            Ai_Aggro_CanExit,
            Ai_Aggro_CanEnter,
        },
};

const AiStateFuncs ai_funcs[AIKIND_COUNT][AISTATE_COUNT] = {
    [AIKIND_WIZARD][AISTATE_AGGRO] =
        {
            .update = Ai_Wizard_Aggro_Update,
        },
};

int Find_Target(World *world, int id, ScAi *ai, ScCmd *cmd, int team)
{
    int bestId       = 0;
    float aggroRange = ai->aggroRange;
    float distSq     = aggroRange * aggroRange;
    vec3s head       = Sol_Body3_GetHead(world, id);

    SparseSet_ScTeam *set_team = Sol_Comp_Set(world, ScTeam);
    for (int i = 0; i < set_team->cnt; i++)
    {
        int idB = set_team->dense[i];
        if (id == idB)
            continue;
        ScTeam *teamB = &set_team->data[i];
        if (team != 0 && teamB->team == team)
            continue;
        Xform xformB = Xform_Get(world, idB);
        vec3s to_ent = glms_vec3_sub(xformB.pos, head);
        float dist2  = glms_vec3_norm2(to_ent);
        if (dist2 <= distSq)
        {
            float inv_dist = 1.0f / aggroRange;

            SolRay ray          = {.start = head,
                                   .dir   = glms_vec3_scale(to_ent, inv_dist),
                                   .dist  = aggroRange,
                                   .mask  = COLLAYER_WORLD};
            SolRayResult result = {0};
            bool hit            = false;
            if (solState.debug)
                hit = Sol_Raycast1D(world, ray, &result, 0.2f);
            else
                hit = Sol_Raycast1(world, ray, &result);
            if (hit)
                bestId = idB;
        }
    }
    return bestId;
}

void Ai_Wizard_Aggro_Update(World *world, int id, ScAi *ai, float dt)
{
    sollog("Wizard Aggro");
}