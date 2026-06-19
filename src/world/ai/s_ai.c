/*
 * File: s_aicontroller.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-19
 *
 */
#include "s_ai.h"
#include "controller/s_controller.h"
#include "sol_core.h"
#include "world.h"
#include "sol_math.h"
#include "replication/s_replication.h"
#include "movement/s_movement.h"
#include "xform/s_xform.h"
#include "vital/s_vital.h"
#include "physx./s_body.h"
#include "owner/s_owner.h"

#include "render/render.h"

static void AiController_Step(World *world, double dt, double time);
static void AiController_Debug(World *world, double dt, double time);

void Sol_Ai_Init(World *world)
{
    world->aicontrollers = calloc(MAX_ENTS, sizeof(CompAi));
    WAddStep(world)      = AiController_Step;
    // WAdd3d(world)        = AiController_Debug;
}

void Sol_Ai_Clear(World *world, int id)
{
    world->masks[id] &= ~HAS_AICONTROLLER;
    memset(&world->aicontrollers[id], 0, sizeof(CompAi));
}

void Sol_Ai_Add(World *world, int id, AiKind kind)
{
    if (!(world->masks[id] & HAS_AICONTROLLER))
        memset(&world->aicontrollers[id], 0, sizeof(CompAi));
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
        CompController *controller   = &world->controllers[id];
        CompAi         *aicontroller = &world->aicontrollers[id];

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

        u32 target = aicontroller->target;
        if (target > 0)
        {
            vec3s myPos        = Sol_Xform_GetPos(world, id);
            vec3s targetPos    = Sol_Xform_GetPos(world, target);
            vec3s dir          = glms_vec3_sub(targetPos, myPos);
            float distToTarget = glms_vec3_norm(dir);

            aicontroller->distToTarget = distToTarget;
            aicontroller->dirToTarget  = glms_vec3_normalize(dir);
        }

        aistate_func[aicontroller->state].update(world, id, dt);
        aicontroller->stateData[aicontroller->state].elapsed += dt;

        aicontroller->justHitUs = 0;
    }
}

static void AiController_Debug(World *world, double dt, double time)
{
    int required = HAS_ACTIVE | HAS_AICONTROLLER;
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if (Sol_Vital_GetDead(world, id) || (world->masks[id] & required) != required ||
            world->replications[id].auth == NETAUTH_REMOTE)
            continue;
        CompController *controller   = &world->controllers[id];
        CompAi         *aicontroller = &world->aicontrollers[id];
        vec3s           pos          = Sol_Xform_GetDrawXform(world, id).pos;
        pos.y += Sol_Physx_GetDims(world, id).y;

        char buffer[12];
        snprintf(buffer, sizeof(buffer), "Target: %d", aicontroller->target);
        Sol_Render_DrawText3D((Text3DDesc){
            .billboard = true, .color = {0, 1, 0, 1}, .pos = pos, .size = 0.4f, .text = buffer, .font = SOL_FONT_ICE});
    }
}

bool Ai_SetState(World *world, int id, AiState nextState, u32 slot)
{
    CompAi *aicontroller = &world->aicontrollers[id];
    if (aicontroller->state == nextState)
        return false;
    const StateFunc *prevfunc = &aistate_func[aicontroller->state];
    if (!prevfunc->canExit(world, id, nextState))
        return false;
    const StateFunc *nextfunc = &aistate_func[nextState];
    if (!nextfunc->canEnter(world, id, aicontroller->state, (u32)nextState, slot))
        return false;

    prevfunc->exit(world, id);
    aicontroller->state                                      = nextState;
    aicontroller->stateData[aicontroller->state].elapsed     = 0;
    aicontroller->stateData[aicontroller->state].accum       = 0;
    aicontroller->stateData[aicontroller->state].lastEntered = solState.gameTime;

    nextfunc->enter(world, id);

    return true;
}

u32 AiController_FindTarget(World *world, int id)
{
    CompAi *aicontroller = &world->aicontrollers[id];
    if (aicontroller->justHitUs > 0)
        return aicontroller->justHitUs;
    float closestDistance = 9999999.0f;
    int   closestTarget   = 0;
    for (int i = 0; i < world->activeCount; i++)
    {
        int otherId = world->activeEntities[i];
        if (!Sol_Owner_GetHostile(world, id, otherId) || Sol_Vital_GetDead(world, otherId) ||
            !(world->masks[otherId] & HAS_VITAL))
            continue;
        vec3s pos       = Sol_Xform_GetPos(world, id);
        vec3s targetPos = Sol_Xform_GetPos(world, otherId);
        float dist      = glms_vec3_distance(pos, targetPos);
        if (dist < 25.0f && dist < closestDistance)
        {
            closestDistance = dist;
            closestTarget   = otherId;
        }
    }
    if (closestTarget > 0)
        return closestTarget;
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
