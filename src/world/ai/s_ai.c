/*
 * File: s_aicontroller.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-19
 *
 */
#include "si_ai.h"
#include "sol_core.h"
#include "world.h"
#include "sol_math.h"
#include "render/render.h"

#include "replication/s_replication.h"
#include "movement/s_movement.h"
#include "xform/s_xform.h"
#include "combat/s_combat.h"
#include "physx/s_body.h"
#include "owner/s_owner.h"
#include "controller/s_controller.h"

typedef struct
{
    int     cnt, cap;
    int    *sparse, *dense;
    CompAi *ais;
} WorldAis;

static void Step(World *world, double dt, double time)
{
    static int required = BITC(HAS_ACTIVE) | BITC(HAS_AI);
    WorldAis  *wc       = world->dense_components[WORLD_SYS_AI];

    for (int i = 0; i < wc->cnt; i++)
    {
        int             id         = wc->dense[i];
        CompAi         *ai         = &wc->ais[i];
        CompController *controller = Sol_Controller_Get(world, id);
        controller->aimpos         = Sol_Physx_GetHeadPos(world, id);
        controller->wishdir        = (vec3s){0, 0, 0};

        // Set all AI forwards
        vec3s lookdir     = controller->lookdir;
        float yaw         = atan2f(lookdir.x, lookdir.z);
        float pitch       = asinf(lookdir.y);
        controller->yaw   = yaw;
        controller->pitch = pitch;
        Sol_Xform_SetYaw(world, id, yaw);
        controller->aimdir = lookdir;

        u32 target = ai->target;
        if (target > 0)
        {
            vec3s myPos        = Sol_Xform_GetPos(world, id);
            vec3s targetPos    = Sol_Xform_GetPos(world, target);
            vec3s dir          = glms_vec3_sub(targetPos, myPos);
            float distToTarget = glms_vec3_norm(dir);

            ai->distToTarget = distToTarget;
            ai->dirToTarget  = glms_vec3_normalize(dir);
        }

        aistate_func[ai->state].update(world, id, dt);
        ai->stateData[ai->state].elapsed += dt;

        ai->justHitUs = 0;
    }
    // for (int i = 0; i < world->activeCount; i++)
    // {
    //     int id = world->activeEntities[i];
    //     if (Sol_Combat_GetDead(world, id) || !WHas(world, id, required) ||
    //         world->replications[id].auth == NETAUTH_REMOTE)
    //         continue;
    //     CompController *controller   = Sol_Controller_Get(world, id);
    //     CompAi         *aicontroller = &world->aicontrollers[id];

    //     controller->aimpos = Sol_Physx_GetHeadPos(world, id);

    //     memset(&controller->wishdir, 0, sizeof(vec3s));

    //     // Set all AI forwards
    //     vec3s lookdir     = controller->lookdir;
    //     float yaw         = atan2f(lookdir.x, lookdir.z);
    //     float pitch       = asinf(lookdir.y);
    //     controller->yaw   = yaw;
    //     controller->pitch = pitch;
    //     Sol_Xform_SetYaw(world, id, yaw);
    //     controller->aimdir = lookdir;

    //     u32 target = aicontroller->target;
    //     if (target > 0)
    //     {
    //         vec3s myPos        = Sol_Xform_GetPos(world, id);
    //         vec3s targetPos    = Sol_Xform_GetPos(world, target);
    //         vec3s dir          = glms_vec3_sub(targetPos, myPos);
    //         float distToTarget = glms_vec3_norm(dir);

    //         aicontroller->distToTarget = distToTarget;
    //         aicontroller->dirToTarget  = glms_vec3_normalize(dir);
    //     }

    //     aistate_func[aicontroller->state].update(world, id, dt);
    //     aicontroller->stateData[aicontroller->state].elapsed += dt;

    //     aicontroller->justHitUs = 0;
    // }
}

void Sol_Ai_Init(World *world)
{
    WorldAis *wc                          = malloc(sizeof(WorldAis));
    world->dense_components[WORLD_SYS_AI] = wc;

    wc->cap    = 64;
    wc->cnt    = 0;
    wc->sparse = malloc(MAX_ENTS * sizeof(int));
    wc->dense  = malloc(wc->cap * sizeof(int));
    wc->ais    = malloc(wc->cap * sizeof(int));
    memset(wc->sparse, -1, MAX_ENTS * sizeof(int));

    WAddStep(world) = Step;
}

CompAi *Sol_Ai_Add(World *world, int id)
{
    WorldAis *wc = world->dense_components[WORLD_SYS_AI];
    if (wc->sparse[id] != -1)
        return &wc->ais[wc->sparse[id]];
    if (wc->cnt >= wc->cap)
    {
        wc->cap *= 2;
        wc->dense = realloc(wc->dense, wc->cap * sizeof(int));
        wc->ais   = realloc(wc->ais, wc->cap * sizeof(CompAi));
    }
    int dense        = wc->cnt++;
    wc->sparse[id]   = dense;
    wc->dense[dense] = id;
    wc->ais[dense]   = (CompAi){0};
    return &wc->ais[dense];
}

CompAi *Sol_Ai_Get(World *world, int id)
{
    WorldAis *wc = world->dense_components[WORLD_SYS_AI];
    if (wc->sparse[id] == -1)
        return NULL;
    return &wc->ais[wc->sparse[id]];
}

void Sol_Ai_Remove(World *world, int id)
{
    WorldAis *wc = world->dense_components[WORLD_SYS_AI];
    for (int i = 0; i < wc->cnt; i++)
    {
        if (wc->dense[i] == id)
        {
            wc->sparse[id] = -1;
            wc->ais[i]     = wc->ais[--wc->cnt];
            WRemB(world, id, HAS_AI);
        }
    }
}

void Ai_Debug(World *world, double dt, double time)
{
    static int required = BITC(HAS_ACTIVE) | BITC(HAS_AI);
    if (!solState.debug)
        return;
    WorldAis *wc = world->dense_components[WORLD_SYS_AI];

    for (int i = 0; i < wc->cnt; i++)
    {
        int             id         = wc->dense[i];
        CompAi         *ai         = &wc->ais[i];
        CompController *controller = Sol_Controller_Get(world, id);

        if (Sol_Combat_GetDead(world, id))
            continue;
        vec3s pos = Sol_Xform_GetDrawXform(world, id).pos;
        pos.y += Sol_Physx_GetDims(world, id).y;

        char buffer[12];
        snprintf(buffer, sizeof(buffer), "Target: %d", ai->target);
        Sol_Render_DrawText3D((Text3DDesc){
            .billboard = true, .color = {0, 1, 0, 1}, .pos = pos, .size = 0.4f, .text = buffer, .font = SOL_FONT_ICE});
    }
    // for (int i = 0; i < world->activeCount; i++)
    // {
    //     int id = world->activeEntities[i];
    //     if (Sol_Combat_GetDead(world, id) || !WHas(world, id, required) ||
    //         world->replications[id].auth == NETAUTH_REMOTE)
    //         continue;
    //     CompController *controller   = Sol_Controller_Get(world, id);
    //     CompAi         *aicontroller = &world->aicontrollers[id];
    //     vec3s           pos          = Sol_Xform_GetDrawXform(world, id).pos;
    //     pos.y += Sol_Physx_GetDims(world, id).y;

    //     char buffer[12];
    //     snprintf(buffer, sizeof(buffer), "Target: %d", aicontroller->target);
    //     Sol_Render_DrawText3D((Text3DDesc){
    //         .billboard = true, .color = {0, 1, 0, 1}, .pos = pos, .size = 0.4f, .text = buffer, .font =
    //         SOL_FONT_ICE});
    // }
}

bool Sol_Ai_SetState(World *world, int id, AiState nextState, u32 slot)
{
    CompAi *aicontroller = Sol_Ai_Get(world, id);
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

u32 Sol_Ai_FindTarget(World *world, int id)
{
    CompAi *ai = Sol_Ai_Get(world, id);
    if (ai->justHitUs > 0)
        return ai->justHitUs;
    float closestDistance = 9999999.0f;
    int   closestTarget   = 0;
    for (int i = 0; i < world->activeCount; i++)
    {
        int otherId = world->activeEntities[i];
        if (!Sol_Owner_GetHostile(world, id, otherId) || Sol_Combat_GetDead(world, otherId) ||
            !(world->masks[otherId] & BITC(HAS_COMBAT)))
            continue;
        vec3s pos       = Sol_Xform_GetPos(world, id);
        vec3s targetPos = Sol_Xform_GetPos(world, otherId);
        float dist      = glms_vec3_distance(pos, targetPos);
        if (dist < 25.0f && dist < closestDistance)
        {
            SolRayResult result = Sol_RaycastD(
                world,
                (SolRay){
                    .dist = 25.0f, .pos = pos, .dir = vecNorm(vecSub(targetPos, pos)), .ignoreEnt = id, .mask = 0b01},
                0.2f);
            if (result.hit && result.entId > 0)
                closestTarget = otherId;
            closestDistance = dist;
        }
    }
    if (closestTarget > 0)
        return closestTarget;
    return 0;
}

void Sol_Ai_SetLastHit(World *world, int id, int source, float damage)
{
    CompAi *ai    = Sol_Ai_Get(world, id);
    ai->justHitUs = source;
    ai->lastHit   = damage;
}

void Sol_Ai_TargetDied(World *world, int id, int target)
{
    CompAi *ai = Sol_Ai_Get(world, id);
    if (target == ai->target)
        ai->target = 0;
}
