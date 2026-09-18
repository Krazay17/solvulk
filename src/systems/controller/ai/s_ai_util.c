#include "s_ai.h"
#include "sol_math.h"
#include "world.h"
#include "sol_core.h"


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
            float inv_dist = 1.0f / dist2;

            SolRay ray          = {.start = head,
                                   .dir   = glms_vec3_scale(to_ent, inv_dist),
                                   .dist  = dist2,
                                   .mask  = COLLAYER_WORLD};
            SolRayResult result = {0};
            bool hit            = false;
            if (solState.debug)
                hit = Sol_Raycast1D(world, ray, &result, 0.2f);
            else
                hit = Sol_Raycast1(world, ray, &result);
            if (!hit)
            {
                bestId = idB;
            }
        }
    }
    return bestId;
}