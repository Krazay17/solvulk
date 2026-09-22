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
        brain->dropAggroTimer = 200.0f;
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

// void Fill_Knows(World *world, int id, ScAi *ai, ScCmd *cmd)
// {
//     AiBrain *brain = &ai->brain;
//     ai->knows      = 0;
//     int target     = brain->target;
//     vec3s pos      = world->xform.pos[id];

//     if (brain->target_dist < 5.0f)
//         ai->knows |= AIKNOWS_TARGETCLOSE;
//     else if (brain->target_dist < 12.0f)
//         ai->knows |= AIKNOWS_TARGETMID;
//     else
//         ai->knows |= AIKNOWS_TARGETFAR;
//     if (brain->target_pos.y > pos.y + 1.0f)
//         ai->knows |= AIKNOWS_TARGETHIGH;

//     ScAbility *target_ability = Sol_Comp_Get(world, target, ScAbility);
//     if (target_ability->stateData[target_ability->activeSlot].stage > 0)
//         ai->knows |= AIKNOWS_TARGETATTACK;

//     if (cmd->actionState & (BITC(ACTION_ABILITY1) | BITC(ACTION_ABILITY2)))
//         ai->knows |= AIKNOWS_CHARGING;

//     ScAbility *ability = Sol_Comp_Get(world, id, ScAbility);
//     if (ability)
//     {
//         if (ability->stateData[ability->activeSlot].power >= 1.0f)
//         {
//             ai->knows |= AIKNOWS_CHARGINGLONG;
//         }
//         if (ability->stateData[6].cooldownRemaining > 0.0f)
//         {
//             ai->knows |= AIKNOWS_DODGECOOLDOWN;
//         }
//     }

//     SparseSet_ScProjectile *projectile_set = Sol_Comp_Set(world, ScProjectile);
//     for (int i = 0; i < projectile_set->cnt; i++)
//     {
//         int projectile_id    = projectile_set->dense[i];
//         vec3s projectile_pos = world->xform.pos[projectile_id];
//         vec3s delta          = glms_vec3_sub(projectile_pos, pos);
//         float d2             = glms_vec3_norm2(delta);
//         if (d2 <= 0.0001f || d2 > 200.0f)
//             continue;
//         float dot   = vecDot(delta, cmd->leftdir);
//         float inv_d = 1.0f / sqrtf(d2);
//         dot *= inv_d;
//         if (dot > 0.0f)
//             ai->knows |= AIKNOWS_DANGERLEFT;
//         else
//             ai->knows |= AIKNOWS_DANGERRIGHT;
//     }

//     ScMove3 *move3 = Sol_Comp_Get(world, id, ScMove3);
//     if (move3)
//     {
//         if (move3->airtime > 0)
//             ai->knows |= AIKNOWS_AIRBORNE;
//     }

//     ScBody3 *body3 = Sol_Comp_Get(world, id, ScBody3);
//     if (body3)
//     {
//         float height   = body3->dims.y;
//         float radius   = body3->dims.x;
//         vec3s fwd      = Sol_Vec3_FromYawPitch(cmd->yaw, 0);
//         vec3s foot_pos = pos;

//         foot_pos.y -= body3->dims.y * 0.8f;

//         SolRayResult result;
//         if (!Sol_Raycast1(world,
//                           (SolRay){
//                               .start     = Sol_Body3_GetHead(world, id),
//                               .mask      = COLLAYER_WORLD,
//                               .ignoreEnt = id,
//                               .dir       = cmd->lookdir,
//                               .dist      = ai->brain.target_dist,
//                           },
//                           &result))
//             ai->knows |= AIKNOWS_TARGETLOS;

//         u32 hitmap[4] = {AIKNOWS_WALLFRONT, AIKNOWS_WALLBACK, AIKNOWS_WALLRIGHT, AIKNOWS_WALLLEFT};
//         for (int k = -1; k < 1; k++)
//         {
//             for (int j = 1; j < 5; j++)
//             {
//                 vec3s finalPos = pos;
//                 finalPos.y += (height * 0.8f) * k;
//                 float final_dist     = radius + 3.5f;
//                 vec3s rotated_offset = glms_quat_rotatev(world->xform.rot[id], VECTOR_RADIAL_DIRECTIONS[j]);
//                 SolRay ray = {.start = finalPos, .dist = final_dist, .dir = rotated_offset, .ignoreEnt = id, .mask =
//                 1}; bool hit   = Sol_Raycast1(world, ray, &result); if (hit)
//                 {
//                     ai->knows |= hitmap[j - 1];
//                 }
//                 if (k < 0)
//                     if (!Sol_Raycast1(world,
//                                       (SolRay){
//                                           .start     = vecAdd(finalPos, vecSca(rotated_offset, final_dist)),
//                                           .dir       = WORLD_DOWN,
//                                           .dist      = 5.0f,
//                                           .ignoreEnt = id,
//                                       },
//                                       &result))
//                         ai->knows |= AIKNOWS_LEDGENEAR;
//             }
//         }
//     }
// }

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
                ai->reward += event->as.hit.damage;
            }
            else if (event->entB == id)
                ai->reward -= event->as.hit.damage;
        }
        else if (event->kind == EVENTKIND_DEATH)
        {
            if (event->entA == id)
                ai->reward -= 10.0f;
            else if (event->entB == id)
                ai->reward += 10.0f;
        }
    }

    AiBrain *brain      = &ai->brain;
    float closer_reward = (brain->target_prev_dist - brain->target_dist);
    ai->reward += closer_reward;

    // if (ai->knows & AIKNOWS_WALLLEFT)
    // {
    //     switch (ai->aiaction)
    //     {
    //     case AIACTION_JUMPLEFT:
    //         ai->reward += 5.0f * fdt;
    //         break;
    //     }
    // }
    // if (ai->knows & AIKNOWS_WALLRIGHT)
    // {
    //     switch (ai->aiaction)
    //     {
    //     case AIACTION_JUMPRIGHT:
    //         ai->reward += 5.0f * fdt;
    //         break;
    //     }
    // }
    // if (ai->knows & AIKNOWS_WALLFRONT)
    // {
    //     switch (ai->aiaction)
    //     {
    //     case AIACTION_JUMPFWD:
    //         ai->reward += 10.0f * fdt;
    //         break;
    //     }
    // }
    // if (ai->knows & AIKNOWS_DANGERLEFT)
    // {
    //     switch (ai->aiaction)
    //     {
    //     case AIACTION_DODGERIGHT:
    //         ai->reward += 5.0f * fdt;
    //         break;
    //     }
    // }
    // if (ai->knows & AIKNOWS_DANGERRIGHT)
    // {
    //     switch (ai->aiaction)
    //     {
    //     case AIACTION_DODGELEFT:
    //         ai->reward += 5.0f * fdt;
    //         break;
    //     }
    // }
}

// Combat Selection
u32 Q_SelectCombatAction(QTable *qt, u32 state, float epsilon)
{
    if (Sol_Math_RandRange2(0.0f, 100.0f) < epsilon)
    {
        return rand() % AIACTION_COUNT;
    }

    u32 best_action = 0;
    float best_q    = -FLT_MAX;

    for (int a = 0; a < AIACTION_COUNT; a++)
    {
        float q_val = qt->q[state][a];
        if (q_val > best_q)
        {
            best_q      = q_val;
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

u32 Ai_GetKnowState(World *world, int id, ScAi *ai, ScCmd *cmd)
{
    AiStateInputs inputs = {0};

    AiBrain *brain = &ai->brain;
    int target     = brain->target;
    vec3s pos      = world->xform.pos[id];

    if (brain->target_dist < 5.0f)
        inputs.dist = AITARGETDIST_CLOSE;
    else if (brain->target_dist < 12.0f)
        inputs.dist = AITARGETDIST_MID;
    else
        inputs.dist = AITARGETDIST_FAR;

    if (brain->target_pos.y > pos.y + 1.0f)
        inputs.height = AIHEIGHT_ABOVE;
    else if (brain->target_pos.y < pos.y - 1.0f)
        inputs.height = AIHEIGHT_BELOW;
    else
        inputs.height = AIHEIGHT_SAME;

    ScAbility *target_ability = Sol_Comp_Get(world, target, ScAbility);
    if (target_ability && target_ability->stateData[target_ability->activeSlot].stage > 0)
        inputs.targetCombat = AITARGET_FIRING;

    ScAbility *ability = Sol_Comp_Get(world, id, ScAbility);
    if (ability)
    {
        if (ability->stateData[ability->activeSlot].power >= 1.0f)
        {
            inputs.self = AISELF_CHARGINGLONG;
        }
        else if (ability->stateData[ability->activeSlot].power > 0.0f)
            inputs.self = AISELF_CHARGING;
        else if (ability->stateData[6].cooldownRemaining <= 0.0f)
        {
            inputs.self = AISELF_CANDODGE;
        }
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
        min_d2      = d2;
        float dot   = vecDot(delta, cmd->leftdir);
        float inv_d = 1.0f / sqrtf(d2);
        dot *= inv_d;
        if (dot > 0.0f)
            inputs.knows |= AIKNOWS_DANGERLEFT;
        else
            inputs.knows |= AIKNOWS_DANGERRIGHT;
    }

    ScMove3 *move3 = Sol_Comp_Get(world, id, ScMove3);
    if (move3)
    {
        if (move3->airtime > 0)
            inputs.knows |= AIKNOWS_AIRBORNE;
    }

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
                inputs.motion = AIMOTION_TOWARD;
            }
            else if (approach_speed > 1.0f)
            {
                inputs.motion = AIMOTION_AWAY;
            }
            else
                inputs.motion = AIMOTION_STILL;
        }
    }

    ScBody3 *body3 = Sol_Comp_Get(world, id, ScBody3);
    if (body3)
    {
        float height = body3->dims.y;
        float radius = body3->dims.x;
        vec3s fwd    = Sol_Vec3_FromYawPitch(cmd->yaw, 0);

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
            inputs.knows |= AIKNOWS_TARGETLOS;

        u32 hitmap[4] = {AIKNOWS_WALLFRONT, AIKNOWS_WALLBACK, AIKNOWS_WALLRIGHT, AIKNOWS_WALLLEFT};
        for (int k = -1; k < 1; k++)
        {
            for (int j = 1; j < 5; j++)
            {
                vec3s finalPos = pos;
                finalPos.y += (height * 0.8f) * k;
                float final_dist     = radius + 3.5f;
                vec3s rotated_offset = glms_quat_rotatev(world->xform.rot[id], VECTOR_RADIAL_DIRECTIONS[j]);
                SolRay ray = {.start = finalPos, .dist = final_dist, .dir = rotated_offset, .ignoreEnt = id, .mask = 1};
                bool hit   = Sol_Raycast1(world, ray, &result);
                if (hit)
                {
                    inputs.knows |= hitmap[j - 1];
                }
                if (k < 0)
                    if (!Sol_Raycast1(world,
                                      (SolRay){
                                          .start     = vecAdd(finalPos, vecSca(rotated_offset, final_dist)),
                                          .dir       = WORLD_DOWN,
                                          .dist      = 5.0f,
                                          .ignoreEnt = id,
                                      },
                                      &result))
                        inputs.knows |= AIKNOWS_LEDGENEAR;
            }
        }
    }

    return Ai_GetStateIndex(&inputs);
}
void Submit_Learn(World *world, int id, ScAi *ai, ScCmd *cmd)
{
    // AiKnows known = ai->knows;
    u32 knows = Ai_GetKnowState(world, id, ai, cmd);
    if (ai->hasPrevKnows)
    {
        Q_Learn_Table(&solData.qtable, ai->prev_knows, ai->prev_action, knows, ai->reward, 0.3f, 0.95f);
    }
    ai->reward       = 0;
    u32 next_action  = Q_SelectCombatAction(&solData.qtable, knows, 10.0f);
    ai->prev_knows   = knows;
    ai->prev_action  = next_action;
    ai->aiaction     = next_action;
    ai->hasPrevKnows = true;
}

const u32 slot_action[4] = {ACTION_ABILITY3, ACTION_ABILITY4, ACTION_ABILITY5, ACTION_ABILITY6};
void Convert_AiActions(ScAi *ai, ScCmd *cmd)
{
    cmd->actionState &= (BITC(ACTION_ABILITY1) | BITC(ACTION_ABILITY2));
    cmd->isStrafing = true;
    AiBrain *brain  = &ai->brain;
    vec3s fwd       = cmd->lookdir;
    vec3s bwd       = glms_vec3_scale(fwd, -1.0f);
    vec3s left      = cmd->leftdir;
    vec3s right     = glms_vec3_scale(left, -1.0f);

    switch (ai->aiaction)
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
        cmd->wishdir = (ai->aiaction == AIACTION_JUMPBWD)     ? bwd
                       : (ai->aiaction == AIACTION_JUMPLEFT)  ? left
                       : (ai->aiaction == AIACTION_JUMPRIGHT) ? right
                                                              : fwd;
        break;
    case AIACTION_CROUCHFWD:
    case AIACTION_CROUCHBWD:
    case AIACTION_CROUCHLEFT:
    case AIACTION_CROUCHRIGHT:
        cmd->actionState |= BITC(ACTION_CROUCH);
        cmd->wishdir = (ai->aiaction == AIACTION_CROUCHBWD)     ? bwd
                       : (ai->aiaction == AIACTION_CROUCHLEFT)  ? left
                       : (ai->aiaction == AIACTION_CROUCHRIGHT) ? right
                                                                : fwd;

        break;
    case AIACTION_DODGEFWD:
    case AIACTION_DODGEBWD:
    case AIACTION_DODGELEFT:
    case AIACTION_DODGERIGHT:
        cmd->actionState |= BITC(ACTION_DASH);
        cmd->wishdir = (ai->aiaction == AIACTION_DODGEBWD)     ? bwd
                       : (ai->aiaction == AIACTION_DODGELEFT)  ? left
                       : (ai->aiaction == AIACTION_DODGERIGHT) ? right
                                                               : fwd;
        break;
    case AIACTION_CHARGE:
        cmd->actionState |= rand() % 2 ? BITC(ACTION_ABILITY1) : BITC(ACTION_ABILITY2);
        ai->actionTimer = 0;
        break;
    case AIACTION_RELEASE:
        cmd->actionState &= ~(BITC(ACTION_ABILITY1) | BITC(ACTION_ABILITY2));
        break;
    case AIACTION_ABILITY:
        u32 slot = rand() % 4;
        cmd->actionState |= BITC(slot_action[slot]);
        break;
    }
}