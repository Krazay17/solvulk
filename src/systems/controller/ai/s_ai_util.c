#include "s_ai.h"
#include "sol_math.h"
#include "world.h"
#include "sol_core.h"

int Find_Target(World *world, int id, ScAi *ai, ScCmd *cmd, int team)
{
    int bestId       = 0;
    float aggroRange = ai->aggroRange;
    float closest_sq = aggroRange * aggroRange; // Track squared distance to avoid premature sqrtf
    vec3s head       = Sol_Body3_GetHead(world, id);

    SparseSet_ScTeam *set_team = Sol_Comp_Set(world, ScTeam);
    for (int i = 0; i < set_team->cnt; i++)
    {
        int idB = set_team->dense[i];
        if (id == idB)
            continue;
        ScCombat *combat = Sol_Comp_Get(world, idB, ScCombat);
        if (combat)
        {
            if (combat->is_dead)
            {
                ai->brain.dropAggroTimer = 0.0f;
                continue;
            }
        }
        ScTeam *teamB = &set_team->data[i];
        if (team != 0 && teamB->team == team)
            continue;

        Xform xformB = Xform_Get(world, idB);

        vec3s delta = glms_vec3_sub(xformB.pos, head);
        float d2    = glms_vec3_norm2(delta);

        // 1. Skip targets outside current closest range
        if (d2 <= 0.0001f || d2 >= closest_sq)
            continue;

        // 2. Only compute sqrtf for candidates that are actually closer
        float dist = sqrtf(d2);
        vec3s dir  = glms_vec3_scale(delta, 1.0f / dist); // Clean unit direction

        // 3. Trace ray exactly from head to target pos (dist = actual distance)
        SolRay ray = {.start     = head,
                      .dir       = dir,  // Normalized direction vector
                      .dist      = dist, // Max trace distance = distance to target
                      .ignoreEnt = id,
                      .mask      = COLLAYER_WORLD};

        SolRayResult result = {0};
        bool hit = solState.debug ? Sol_Raycast1D(world, ray, &result, 0.2f) : Sol_Raycast1(world, ray, &result);

        // If no world obstruction, this is our new best target
        if (!hit)
        {
            closest_sq = d2;
            bestId     = idB;
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
        brain->dropAggroTimer = 1e9f;
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
        vec3s target_pos        = world->xform.pos[brain->target];
        brain->target_pos       = target_pos;
        brain->target_dir       = vecNorm(vecSub(target_pos, pos));
        brain->target_prev_dist = brain->target_dist;
        brain->target_dist      = glms_vec3_distance(target_pos, pos);
    }
}

void Fill_Reward(World *world, int id, ScAi *ai, float fdt)
{
    SlEvent *events = Sol_Comp_Get(world, 0, SlEvent);
    int count       = solb_count(events->events);
    for (int i = 0; i < count; i++)
    {
        SolEvent *event = &events->events[i];
        if (event->kind == EVENTKIND_HIT)
        {
            if (event->entA == id)
            {
                ai->learning.reward_move += event->as.hit.damage;
                ai->learning.reward_combat += event->as.hit.damage;
            }
            else if (event->entB == id)
            {
                ai->learning.reward_move -= event->as.hit.damage;
                ai->learning.reward_combat -= event->as.hit.damage;
            }
        }
        else if (event->kind == EVENTKIND_DEATH)
        {
            if (event->entA == id)
            {
                ai->learning.reward_move -= 10.0f;
                ai->learning.reward_combat -= 10.0f;
            }
            else if (event->entB == id)
            {
                ai->learning.reward_move += 10.0f;
                ai->learning.reward_combat += 10.0f;
            }
        }
    }

    AiBrain *brain = &ai->brain;
    vec3s last_pos = world->xform.last_pos[id];
    vec3s pos      = world->xform.pos[id];

    float closer_reward = (brain->target_prev_dist - brain->target_dist);
    ai->learning.reward_move += closer_reward * 0.5f;
    ai->learning.reward_move -= brain->target_dist * 0.5f * fdt;
}

u32 GetCombatActionMask(World *world, int id, ScAi *ai)
{
    u32 mask         = 0;
    bool is_charging = (ai->learning.prev_knows_combat.self >= 2);
    if (is_charging)
    {
        mask |= BITC(AIACTIONC_NONE);
        mask |= BITC(AIACTIONC_RELEASE);
    }
    else
    {
        mask |= BITC(AIACTIONC_NONE);
        mask |= BITC(AIACTIONC_CHARGE);
        // mask |= BITC(AIACTION_ABILITY);
    }
    return mask;
}
u32 GetMoveActionMask(World *world, int id, ScAi *ai)
{
    u32 mask = 0;
    mask |= BITC(AIACTION_NONE);
    mask |= BITC(AIACTION_FWD) | BITC(AIACTION_BWD) | BITC(AIACTION_LEFT) | BITC(AIACTION_RIGHT);
    mask |= BITC(AIACTION_JUMPFWD) | BITC(AIACTION_JUMPBWD) | BITC(AIACTION_JUMPLEFT) | BITC(AIACTION_JUMPRIGHT);
    mask |=
        BITC(AIACTION_CROUCHFWD) | BITC(AIACTION_CROUCHBWD) | BITC(AIACTION_CROUCHLEFT) | BITC(AIACTION_CROUCHRIGHT);

    ScAbility *ability = Sol_Comp_Get(world, id, ScAbility);
    if (ability && ability->stateData[6].cooldownRemaining <= 0.0f)
    {
        mask |=
            BITC(AIACTION_DODGEFWD) | BITC(AIACTION_DODGEBWD) | BITC(AIACTION_DODGELEFT) | BITC(AIACTION_DODGERIGHT);
    }
    return mask;
}

u32 Q_SelectAction_Array(float *q_table, int num_actions, u32 state, float epsilon, u32 valid_mask)
{
    u32 valid_actions[32];
    int valid_count = 0;

    for (int a = 0; a < num_actions; a++)
    {
        if (valid_mask & BITC(a))
            valid_actions[valid_count++] = a;
    }

    if (valid_count == 0)
        return 0;

    // Exploration
    if (((float)rand() / (float)RAND_MAX) < epsilon)
        return valid_actions[rand() % valid_count];

    // Exploitation
    float *state_row = &q_table[state * num_actions];
    u32 best_action  = valid_actions[0];
    float best_q     = -FLT_MAX;

    for (int i = 0; i < valid_count; i++)
    {
        u32 a = valid_actions[i];
        if (state_row[a] > best_q)
        {
            best_q      = state_row[a];
            best_action = a;
        }
    }

    return best_action;
}

void Q_Learn_Table(QTable *qt, u32 state, u32 action, u32 next_state, float reward, float alpha, float gamma)
{
    float max_q_next = -FLT_MAX;
    for (int na = 0; na < AIACTION_COUNT; na++)
    {
        float q_next = qt->q[next_state][na];
        if (q_next > max_q_next)
        {
            max_q_next = q_next;
        }
    }

    float current_q = qt->q[state][action];
    float target_q  = reward + gamma * max_q_next;

    qt->q[state][action] += alpha * (target_q - current_q);
}

static const float dist_map[7] = {2.0f, 5.0f, 8.0f, 12.0f, 14.0f, 18.0f, 22.0f};
AiKnowStateM Get_KnowsM(World *world, int id, ScAi *ai, ScCmd *cmd)
{
    AiKnowStateM knows = {0};
    AiBrain *brain     = &ai->brain;
    vec3s pos          = world->xform.pos[id];
    int target         = brain->target;
    vec3s target_pos   = world->xform.pos[target];
    knows.targetDist   = 7;
    for (int i = 0; i < 7; i++)
    {
        if (brain->target_dist < dist_map[i])
        {
            knows.targetDist = i;
            break;
        }
    }

    ScAbility *ability = Sol_Comp_Get(world, id, ScAbility);
    if (ability)
    {
        if (ability->stateData[ability->activeSlot].power >= 0.8f)
            knows.self = 3;
        else if (ability->stateData[ability->activeSlot].power > 0.0f)
            knows.self = 2;
        else if (ability->stateData[6].cooldownRemaining > 0.0f)
            knows.self = 1;
    }

    SparseSet_ScProjectile *projectile_set = Sol_Comp_Set(world, ScProjectile);
    float min_d2                           = 200.0f;
    for (int i = 0; i < projectile_set->cnt; i++)
    {
        int projectile_id    = projectile_set->dense[i];
        vec3s projectile_pos = world->xform.pos[projectile_id];
        vec3s delta          = glms_vec3_sub(projectile_pos, pos);
        float d2             = glms_vec3_norm2(delta);
        if (d2 <= 0.0001f || d2 > min_d2)
            continue;
        min_d2       = d2;
        knows.danger = 1;
        // float dot   = vecDot(delta, cmd->leftdir);
        // float inv_d = 1.0f / sqrtf(d2);
        // dot *= inv_d;
        // if (dot > 0.0f)
        //     knows.dangerLeft = 1;
        // else
        //     knows.dangerRight = 1;
    }
    ScMove3 *move3 = Sol_Comp_Get(world, id, ScMove3);
    if (move3)
    {
        if (move3->state == MOVE_SLIDE)
            knows.moveState = 3;
        else if (move3->state == MOVE_WALLRUN)
            knows.moveState = 2;
        else if (move3->state == MOVE_FALL)
            knows.moveState = 1;
    }

    // TARGET STATE
    ScBody3 *target_body = Sol_Comp_Get(world, target, ScBody3);
    if (target_body)
    {
        vec3s target_vel = target_body->vel; // or derived from position delta
        vec3s to_target  = glms_vec3_sub(brain->target_pos, pos);
        float dist_sq    = glms_vec3_norm2(to_target);

        if (dist_sq > 0.0001f)
        {
            vec3s dir_to_target  = glms_vec3_scale(to_target, 1.0f / sqrtf(dist_sq));
            float approach_speed = glms_vec3_dot(target_vel, dir_to_target);

            // Negative dot product means velocity points back toward AI
            if (approach_speed < -1.0f)
            {
                knows.targetState = 1;
            }
            else if (approach_speed > 1.0f)
            {
                knows.targetState = 2;
            }
        }
    }
    if ((pos.y + 0.5f) < target_pos.y)
        knows.targetState = 3;
    ScAbility *target_ability = Sol_Comp_Get(world, target, ScAbility);
    if (target_ability)
    {
        if (target_ability->stateData[target_ability->activeSlot].stage > 0)
            knows.targetState = 4;
        else if (target_ability->stateData[target_ability->activeSlot].power > 0.0f)
            knows.targetState = 5;
        else if (target_ability->state == ABILITY_STATE_DASH)
            knows.targetState = 6;
    }

    ScBody3 *body3 = Sol_Comp_Get(world, id, ScBody3);
    if (body3)
    {
        float height = body3->dims.y;
        float radius = body3->dims.x;
        vec3s fwd    = Sol_Vec3_FromYawPitch(cmd->yaw, 0);

        SolRayResult result;
        u32 wall_mask = 0;
        for (int j = 1; j < 5; j++)
        {
            vec3s finalPos       = pos;
            float final_dist     = radius * 2.0f;
            vec3s rotated_offset = glms_quat_rotatev(world->xform.rot[id], VECTOR_RADIAL_DIRECTIONS[j]);
            SolRay ray = {.start = finalPos, .dist = final_dist, .dir = rotated_offset, .ignoreEnt = id, .mask = 1};
            bool hit   = Sol_Raycast1(world, ray, &result);
            if (hit)
            {
                wall_mask |= (1 << j - 1);
            }
            else if (j == 1 || j == 2)
                if (!Sol_Raycast1(world,
                                  (SolRay){
                                      .start     = vecAdd(finalPos, vecSca(rotated_offset, final_dist)),
                                      .dir       = WORLD_DOWN,
                                      .dist      = 5.0f,
                                      .ignoreEnt = id,
                                  },
                                  &result))
                    knows.surrounding = j == 1 ? 1 : 2;
        }
        if (glms_vec3_norm(body3->vel) < 1.0f && glms_vec3_norm(cmd->wishdir) > 0.001f)
            knows.surrounding = 3;
        knows.wallFront = (wall_mask >> 0) & 1;
        knows.wallBack  = (wall_mask >> 1) & 1;
        knows.wallRight = (wall_mask >> 2) & 1;
        knows.wallLeft  = (wall_mask >> 3) & 1;
    }

    return knows;
}
AiKnowStateC Get_KnowsC(World *world, int id, ScAi *ai, ScCmd *cmd)
{
    AiKnowStateC knows = {0};
    AiBrain *brain     = &ai->brain;
    vec3s pos          = world->xform.pos[id];
    int target         = brain->target;
    vec3s target_pos   = world->xform.pos[target];
    knows.targetDist   = 7;
    for (int i = 0; i < 7; i++)
    {
        if (brain->target_dist < dist_map[i])
        {
            knows.targetDist = i;
            break;
        }
    }

    ScAbility *ability = Sol_Comp_Get(world, id, ScAbility);
    if (ability)
    {
        if (ability->stateData[ability->activeSlot].power >= 0.8f)
            knows.self = 3;
        else if (ability->stateData[ability->activeSlot].power > 0.0f)
            knows.self = 2;
        else if (ability->state != 0)
            knows.self = 1;
    }

    SolRayResult result;
    if (!Sol_Raycast1(world,
                      (SolRay){
                          .start     = Sol_Body3_GetHead(world, id),
                          .mask      = COLLAYER_WORLD,
                          .ignoreEnt = id,
                          .dir       = cmd->lookdir,
                          .dist      = ai->brain.target_dist,
                      },
                      &result))
        knows.targetLos = 1;

    ScCombat *combat        = Sol_Comp_Get(world, id, ScCombat);
    ScCombat *target_combat = Sol_Comp_Get(world, target, ScCombat);
    if (combat && target_combat)
    {
        knows.winning = combat->health > target_combat->health;
    }

    return knows;
}

void Learn_Table(float *table, u32 num_actions, u32 next_knows, u32 action, u32 knows, float reward, float alpha,
                 float gamma)
{
    float *row      = &table[knows * num_actions];
    float *next_row = &table[next_knows * num_actions];
    float max_q     = -FLT_MAX;
    for (int na = 0; na < num_actions; na++)
    {
        if (next_row[na] > max_q)
            max_q = next_row[na];
    }
    float current_q = row[action];
    float target_q  = reward + gamma * max_q;
    row[action] += alpha * (target_q - current_q);
}

void Submit_Learn(World *world, int id, ScAi *ai, ScCmd *cmd)
{
    if (!solData.qtable.q || !solData.qtable.qc)
        return;
    AiKnowStateM next_knows_move   = Get_KnowsM(world, id, ai, cmd);
    AiKnowStateC next_knows_combat = Get_KnowsC(world, id, ai, cmd);
    if (next_knows_move.raw && ai->learning.prev_knows_move.raw)
        Learn_Table((float *)solData.qtable.q, AIACTION_COUNT, next_knows_move.raw, ai->learning.action_move,
                    ai->learning.prev_knows_move.raw, ai->learning.reward_move, AI_ALPHA, AI_GAMMA);
    if (next_knows_combat.raw && ai->learning.prev_knows_combat.raw)
        Learn_Table((float *)solData.qtable.qc, AIACTIONC_COUNT, next_knows_combat.raw, ai->learning.action_combat,
                    ai->learning.prev_knows_combat.raw, ai->learning.reward_combat, AI_ALPHA, AI_GAMMA);
    ai->learning.reward_move   = 0;
    ai->learning.reward_combat = 0;
    ai->learning.action_move =
        Q_SelectAction_Array((float *)solData.qtable.q, AIACTION_COUNT, ai->learning.prev_knows_move.raw, AI_EXPLORE,
                             GetMoveActionMask(world, id, ai));
    ai->learning.action_combat =
        Q_SelectAction_Array((float *)solData.qtable.qc, AIACTIONC_COUNT, ai->learning.prev_knows_combat.raw,
                             AI_EXPLORE, GetCombatActionMask(world, id, ai));
    Convert_AiActions(ai, cmd, next_knows_move, next_knows_combat);
    ai->learning.prev_knows_move.raw   = next_knows_move.raw;
    ai->learning.prev_knows_combat.raw = next_knows_combat.raw;
}

const u32 slot_action[4] = {ACTION_ABILITY3, ACTION_ABILITY4, ACTION_ABILITY5, ACTION_ABILITY6};
void Convert_AiActions(ScAi *ai, ScCmd *cmd, AiKnowStateM next_knows_move, AiKnowStateC next_knows_combat)
{
    cmd->actionState &= (BITC(ACTION_ABILITY1) | BITC(ACTION_ABILITY2));
    cmd->isStrafing = true;
    AiBrain *brain  = &ai->brain;
    vec3s fwd       = cmd->lookdir;
    fwd.y = 0;
    fwd = vecNorm(fwd);
    vec3s bwd       = glms_vec3_scale(fwd, -1.0f);
    vec3s left      = cmd->leftdir;
    vec3s right     = glms_vec3_scale(left, -1.0f);

    switch (ai->learning.action_move)
    {
    case AIACTION_NONE:
        cmd->wishdir = (vec3s){0};
        break;
    case AIACTION_FWD:
        cmd->wishdir = fwd;
        break;
    case AIACTION_BWD:
        cmd->wishdir = bwd;
        break;
    case AIACTION_LEFT:
        cmd->wishdir = left;
        break;
    case AIACTION_RIGHT:
        cmd->wishdir = right;
        break;
    case AIACTION_JUMPFWD:
    case AIACTION_JUMPBWD:
    case AIACTION_JUMPLEFT:
    case AIACTION_JUMPRIGHT:
        cmd->actionState |= BITC(ACTION_JUMP);
        cmd->wishdir             = (ai->learning.action_move == AIACTION_JUMPBWD)     ? bwd
                                   : (ai->learning.action_move == AIACTION_JUMPLEFT)  ? left
                                   : (ai->learning.action_move == AIACTION_JUMPRIGHT) ? right
                                                                                      : fwd;
        ai->learning.actionTimer = 0.4f;
        break;
    case AIACTION_CROUCHFWD:
    case AIACTION_CROUCHBWD:
    case AIACTION_CROUCHLEFT:
    case AIACTION_CROUCHRIGHT:
        cmd->actionState |= BITC(ACTION_CROUCH);
        cmd->wishdir = (ai->learning.action_move == AIACTION_CROUCHBWD)     ? bwd
                       : (ai->learning.action_move == AIACTION_CROUCHLEFT)  ? left
                       : (ai->learning.action_move == AIACTION_CROUCHRIGHT) ? right
                                                                            : fwd;
        break;
    case AIACTION_DODGEFWD:
    case AIACTION_DODGEBWD:
    case AIACTION_DODGELEFT:
    case AIACTION_DODGERIGHT:
        cmd->actionState |= BITC(ACTION_DASH);
        cmd->wishdir             = (ai->learning.action_move == AIACTION_DODGEBWD)     ? bwd
                                   : (ai->learning.action_move == AIACTION_DODGELEFT)  ? left
                                   : (ai->learning.action_move == AIACTION_DODGERIGHT) ? right
                                                                                       : fwd;
        ai->learning.actionTimer = 0.3f;
        break;
    }

    switch (ai->learning.action_combat)
    {
    case AIACTIONC_NONE:
        if (next_knows_combat.self < 2)
        {
            cmd->actionState &= ~(BITC(ACTION_ABILITY1) | BITC(ACTION_ABILITY2));
        }
        break;
    case AIACTIONC_CHARGE:
        cmd->actionState |= rand() % 2 ? BITC(ACTION_ABILITY1) : BITC(ACTION_ABILITY2);
        ai->learning.actionTimer = 0;
        break;
    case AIACTIONC_RELEASE:
        cmd->actionState &= ~(BITC(ACTION_ABILITY1) | BITC(ACTION_ABILITY2));
        if (ai->learning.prev_knows_combat.self == 3)
            ai->learning.reward_combat += 30.0f;
        break;
    case AIACTIONC_ABILITY:
        u32 slot = rand() % 4;
        cmd->actionState |= BITC(slot_action[slot]);
        break;
    }
}