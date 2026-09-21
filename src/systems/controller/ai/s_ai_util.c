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

            SolRay ray = {
                .start = head, .dir = glms_vec3_scale(to_ent, inv_dist), .dist = dist2, .mask = COLLAYER_WORLD};
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

void Fill_Brain(World *world, int id, ScAi *ai, ScCmd *cmd, float fdt)
{
    AiBrain *brain = &ai->brain;
    vec3s pos      = world->xform.pos[id];

    ScTeam *team     = Sol_Comp_Get(world, id, ScTeam);
    int found_target = team ? Find_Target(world, id, ai, cmd, team->team) : 0;
    if (found_target)
    {
        brain->target         = found_target;
        brain->dropAggroTimer = 20.0f;
    }
    else if (brain->target)
    {
        brain->dropAggroTimer -= fdt;
        if (brain->dropAggroTimer <= 0.0f)
        {
            brain->target = 0;
        }
    }
    if (brain->target)
    {
        vec3s target_pos   = world->xform.pos[brain->target];
        brain->target_pos  = target_pos;
        brain->target_dir  = vecNorm(vecSub(target_pos, pos));
        brain->target_dist = glms_vec3_distance(target_pos, pos);
    }
}

void Fill_Knows(World *world, int id, ScAi *ai, ScCmd *cmd)
{
    ai->knows  = 0;
    int target = ai->brain.target;
    vec3s pos  = world->xform.pos[id];

    ScBody3 *body3 = Sol_Comp_Get(world, id, ScBody3);
    if (body3)
    {
        vec3s foot_pos = pos;
        foot_pos.y -= body3->dims.y * 0.8f;
        SolRay ray = {
            .start     = foot_pos,
            .dir       = Sol_Vec3_FromYawPitch(cmd->yaw, 0),
            .dist      = body3->dims.x + 1.0f,
            .ignoreEnt = id,
        };
        SolRayResult result;
        bool hit = Sol_Raycast1(world, ray, &result);
        if (hit)
            ai->knows |= AIKNOWS_STEPFRONT;

        float height = body3->dims.y;
        float radius = body3->dims.x;

        u32 hitmap[4] = {AIKNOWS_WALLFRONT, AIKNOWS_WALLBACK, AIKNOWS_WALLRIGHT, AIKNOWS_WALLLEFT};
        for (int j = 1; j < 5; j++)
        {
            vec3s finalPos       = pos;
            vec3s rotated_offset = glms_quat_rotatev(world->xform.rot[id], VECTOR_RADIAL_DIRECTIONS[j]);
            SolRay ray = {.start = finalPos, .dist = radius + 5.0f, .dir = rotated_offset, .ignoreEnt = id, .mask = 1};
            bool hit   = Sol_Raycast1(world, ray, &result);
            if (hit)
            {
                ai->knows |= hitmap[j - 1];
            }
        }
    }
}

// Combat Selection
int Q_SelectCombatAction(QTable *qt, u32 state, float epsilon)
{
    if (Sol_Math_RandRange2(0.0f, 100.0f) < epsilon)
    {
        return rand() % ACTION_COMBAT_COUNT;
    }

    int best_action = 0;
    float best_q = -FLT_MAX;

    for (int a = 0; a < ACTION_COMBAT_COUNT; a++)
    {
        float q_val = qt->qcombat[state][a];
        if (q_val > best_q)
        {
            best_q = q_val;
            best_action = a;
        }
    }
    return best_action;
}

// Movement Selection
int Q_SelectMoveAction(QTable *qt, u32 state, float epsilon)
{
    if (Sol_Math_RandRange2(0.0f, 100.0f) < epsilon)
    {
        return rand() % ACTION_MOVE_COUNT;
    }

    int best_action = 0;
    float best_q = -FLT_MAX;

    for (int a = 0; a < ACTION_MOVE_COUNT; a++)
    {
        float q_val = qt->qmove[state][a];
        if (q_val > best_q)
        {
            best_q = q_val;
            best_action = a;
        }
    }
    return best_action;
}

void Q_Learn_Table(float *table, int num_actions, u32 s, int a, float reward, u32 s_next, float alpha, float gamma)
{
    // Stride calculation: row index is (state * num_actions)
    int next_state_offset = s_next * num_actions;

    float max_q_next = -FLT_MAX;
    for (int na = 0; na < num_actions; na++)
    {
        float q_next = table[next_state_offset + na];
        if (q_next > max_q_next)
        {
            max_q_next = q_next;
        }
    }

    int current_idx = s * num_actions + a;
    float current_q = table[current_idx];
    float target_q  = reward + gamma * max_q_next;

    table[current_idx] += alpha * (target_q - current_q);
}

void Fill_Learn(World *world, int id, ScAi *ai, ScCmd *cmd, float fdt)
{
    // cmd->actionState
    // ailearn->actionWeights
}
