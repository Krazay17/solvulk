/*
 * File: s_aicontroller.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-19
 *
 */
#include "sol_core.h"

extern const StateFunc state_func[AISTATE_COUNT] = {
    [AISTATE_IDLE] =
        {
            Idle_State_Update,
            Idle_State_Enter,
            Idle_State_Exit,
            Idle_State_CanExit,
            Idle_State_CanEnter,
        },
    [AISTATE_PATROL] =
        {
            Patrol_State_Update,
            Patrol_State_Enter,
            Patrol_State_Exit,
            Patrol_State_CanExit,
            Patrol_State_CanEnter,
        },
    [AISTATE_AGGRO] =
        {
            Aggro_State_Update,
            Aggro_State_Enter,
            Aggro_State_Exit,
            Aggro_State_CanExit,
            Aggro_State_CanEnter,
        },
};

static void AiController_Step(World *world, double dt, double time);

void Sol_AiController_Init(World *world)
{
    WAddStep(world)      = AiController_Step;
    world->aicontrollers = calloc(MAX_ENTS, sizeof(CompAiController));
}

void Sol_AiController_Clear(World *world, int id)
{
    world->masks[id] &= ~HAS_AICONTROLLER;
    memset(&world->aicontrollers[id], 0, sizeof(CompAiController));
}

void Sol_AiController_Add(World *world, int id, AiControllerKind kind)
{
    world->masks[id] |= HAS_AICONTROLLER;
}

static void AiController_Step(World *world, double dt, double time)
{
    int required = HAS_ACTIVE | HAS_AICONTROLLER;
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if (Sol_Vital_GetDead(world, id) || (world->masks[id] & required) != required ||
            world->replications[id].auth == NETAUTH_REMOTE)
            continue;
        CompController   *controller   = &world->controllers[id];
        CompAiController *aicontroller = &world->aicontrollers[id];

        controller->aimpos = Sol_Physx_GetHeadPos(world, id);

        memset(&controller->wishdir, 0, sizeof(vec3s));

        // Set all AI forwards
        vec3s lookdir     = controller->lookdir;
        float yaw         = atan2f(lookdir.x, lookdir.z);
        float pitch       = asinf(lookdir.y);
        controller->yaw   = yaw;
        controller->pitch = pitch;
        Sol_Xform_SetYaw(world, id, yaw);
        controller->aimdir = lookdir;

        // TODO loop through hostiles
        u32 target = aicontroller->target;
        if (target)
        {
            vec3s myPos        = Sol_Xform_GetPos(world, id);
            vec3s targetPos    = Sol_Xform_GetPos(world, target);
            vec3s dir          = glms_vec3_sub(targetPos, myPos);
            float distToTarget = glms_vec3_norm(dir);

            aicontroller->distToTarget = distToTarget;
            aicontroller->dirToTarget  = glms_vec3_normalize(dir);
        }

        state_func[aicontroller->state].update(world, id, dt);
        aicontroller->stateData[aicontroller->state].elapsed += dt;

        aicontroller->justHitUs = 0;
    }
}

bool Ai_SetState(World *world, int id, AiState nextState)
{
    CompAiController *aicontroller = &world->aicontrollers[id];
    if (aicontroller->state == nextState)
        return false;
    const StateFunc *prevfunc = &state_func[aicontroller->state];
    if (!prevfunc->canExit(world, id, nextState))
        return false;
    const StateFunc *nextfunc = &state_func[nextState];
    if (!nextfunc->canEnter(world, id, aicontroller->state, (u32)nextState))
        return false;

    prevfunc->exit(world, id);
    aicontroller->state                                      = nextState;
    aicontroller->stateData[aicontroller->state].elapsed     = 0;
    aicontroller->stateData[aicontroller->state].accum       = 0;
    aicontroller->stateData[aicontroller->state].lastEntered = (float)Sol_GetState()->gameTime;

    nextfunc->enter(world, id);

    return true;
}

u32 AiController_FindTarget(World *world, int id)
{
    CompAiController *aicontroller = &world->aicontrollers[id];
    if (aicontroller->justHitUs)
        return aicontroller->justHitUs;
    for (int i = 0; i < world->activeCount; i++)
    {
        int otherId = world->activeEntities[i];
        if (Sol_Owner_GetHostile(world, id, otherId))
        {
            vec3s pos       = Sol_Xform_GetPos(world, id);
            vec3s targetPos = Sol_Xform_GetPos(world, otherId);
            float dist      = glms_vec3_distance(pos, targetPos);
            if (dist < 25.0f && world->masks[id] & HAS_VITAL && !Sol_Vital_GetDead(world, otherId))
                return otherId;
        }
    }
    return 0;
}

void Sol_AiController_SetLastHit(World *world, int id, int source, u32 damage)
{
    world->aicontrollers[id].justHitUs = source;
    world->aicontrollers[id].lastHit   = damage;
}

void Sol_AiController_TargetDied(World *world, int id, int target)
{
    if (target == world->aicontrollers[id].target)
        world->aicontrollers[id].target = 0;
}