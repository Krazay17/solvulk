/*
 * File: ss_user.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-08-19
 *
 */
#include "s_player.h"
#include "world.h"
#include "sol_user.h"
#include "sol_math.h"
#include "input.h"

#include "xform/s_xform.h"
#include "controller/s_controller.h"
#include "buff/s_buff.h"
#include "camera/s_camera.h"
#include "physx/s_body.h"

typedef struct
{
    int         cnt, cap;
    int        *sparse, *dense;
    CompPlayer *players;
    int         player_slots[MAX_LOCAL_PLAYERS];
} WorldPlayers;

static void Tick(World *world, double dt, double time)
{
    float fdt = (float)dt;

    WorldPlayers *wc = world->dense_components[WORLD_SYS_PLAYER];
    for (int i = 0; i < wc->cnt; i++)
    {
        int             id         = wc->dense[i];
        CompPlayer     *player     = &wc->players[i];
        CompController *controller = Sol_Controller_Get(world, id);
        SolMouse        mouse      = Sol_Input_GetMouse();

        controller->actionState = 0;
        float *yaw              = &controller->yaw;
        float *pitch            = &controller->pitch;

        if (mouse.locked)
        {
            *yaw -= (float)(mouse.dx * user_settings.look_sens);
            *pitch -= (float)(mouse.dy * user_settings.look_sens);

            *yaw = fmodf(*yaw, 2.0f * GLM_PIf);
            if (*yaw > GLM_PIf)
                *yaw -= 2.0f * GLM_PIf;
            else if (*yaw < -GLM_PIf)
                *yaw += 2.0f * GLM_PIf;

            *pitch = glm_clamp(*pitch, -MAX_PITCH, MAX_PITCH);
        }

        for (int i = 0; i < SOL_KEY_COUNT; i++)
        {
            if (Sol_Input_KeyDown(i))
                controller->actionState |= user_settings.key_binds[i];
        }

        controller->isStrafing = mouse.locked;

        if (WHas(world, id, BITC(HAS_BUILDING)))
        {
            if (mouse.buttons[SOL_MOUSE_LEFT])
                controller->actionState |= ACTION_BUILD;
        }
        else
        {
            if (mouse.togglelocked)
            {
                if (mouse.buttons[SOL_MOUSE_LEFT])
                    controller->actionState |= user_settings.mouse_binds[SOL_MOUSE_LEFT];

                if (mouse.buttons[SOL_MOUSE_RIGHT])
                    controller->actionState |= user_settings.mouse_binds[SOL_MOUSE_RIGHT];
            }
            else if (mouse.locked && mouse.buttons[SOL_MOUSE_LEFT])
                controller->actionState |= ACTION_FWD;

            if (mouse.wheelV)
            {
                float changeDist = -((float)mouse.wheelV * 0.01f);
                Sol_Cam_AdjustDistance(world, id, changeDist);
            }
        }
        controller->yaw      = *yaw;
        controller->pitch    = *pitch;
        controller->lookdir  = Sol_Vec3_FromYawPitch(*yaw, *pitch);
        controller->wishdir  = CalcWishdir3(controller->actionState, controller->lookdir, WORLD_UP, false);
        controller->wishdirY = CalcWishdir3(controller->actionState, controller->lookdir, WORLD_UP, true);
        if (WHasB(world, id, HAS_BODY3))
            controller->aimpos = Sol_Physx_GetHeadPos(world, id);
        if (WHasB(world, id, HAS_CAMERA))
            Sol_Controller_SetParallaxAim(world, id, Sol_Cam_Get(world, id)->pos, Sol_Cam_Get(world, id)->dir, 60.0f,
                                          0.5f);

        // #### DEBUG ACTIONS ####
        if (Sol_Input_KeyDown(SOL_KEY_F))
        {
            controller->actionState |= ACTION_DEBUGTELE;
        }
        else
            controller->actionState &= ~ACTION_DEBUGTELE;

        if (Sol_Input_KeyPressed(SOL_KEY_5))
        {
            vec3s pos = Sol_Xform_GetPos(world, id);
            if (!Sol_Buff_HasBuff(world, id, BUFFKIND_INVULN))
                Sol_Buff_AddEx(world, id, id, BUFFKIND_INVULN, 99999.9f, 0);
            else
                Sol_Buff_Remove(world, id, BUFFKIND_INVULN);
        }
        if (Sol_Input_KeyPressed(SOL_KEY_6))
        {
        }
    }
}

void Sol_Player_Init(World *world)
{
    WorldPlayers *wc                          = malloc(sizeof(WorldPlayers));
    world->dense_components[WORLD_SYS_PLAYER] = wc;

    wc->cap     = 1;
    wc->cnt     = 0;
    wc->sparse  = malloc(MAX_ENTS * sizeof(int));
    wc->dense   = malloc(wc->cap * sizeof(int));
    wc->players = malloc(wc->cap * sizeof(CompPlayer));
    memset(wc->sparse, -1, MAX_ENTS * sizeof(int));
    memset(wc->player_slots, -1, sizeof(wc->player_slots));

    WAddTick(world) = Tick;
}

CompPlayer *Sol_Player_Add(World *world, int id, int localIdx)
{
    WorldPlayers *wc = world->dense_components[WORLD_SYS_PLAYER];
    if (wc->sparse[id] != -1)
        return &wc->players[wc->sparse[id]];

    if (!WHasB(world, id, HAS_CONTROLLER))
        Sol_Controller_Add(world, id);

    if (wc->cnt >= wc->cap)
    {
        wc->cap *= 2;
        wc->dense   = realloc(wc->dense, wc->cap * sizeof(int));
        wc->players = realloc(wc->players, wc->cap * sizeof(CompPlayer));
    }

    // Standard contiguous insertion
    int idx        = wc->cnt++;
    wc->sparse[id] = idx;
    wc->dense[idx] = id;

    // Component holds the player index data
    wc->players[idx] = (CompPlayer){
        .localIdx = localIdx,
    };

    // Maintain slot lookup cache
    if (localIdx > -1 && localIdx <= MAX_LOCAL_PLAYERS - 1)
    {
        wc->player_slots[localIdx] = id;
    }

    WAddComp(world, id, HAS_PLAYER);
    return &wc->players[idx];
}

CompPlayer *Sol_Player_Get(World *world, int id)
{
    WorldPlayers *wc = world->dense_components[WORLD_SYS_PLAYER];
    if (wc->sparse[id] < 0)
        return NULL;
    return &wc->players[wc->sparse[id]];
}

void Sol_Player_Remove(World *world, int id)
{
    WorldPlayers *wc  = world->dense_components[WORLD_SYS_PLAYER];
    int           idx = wc->sparse[id];
    if (idx < 0)
        return;

    int slot = wc->players[idx].localIdx;
    if (slot >= 0 && slot < MAX_LOCAL_PLAYERS && wc->player_slots[slot] == id)
    {
        wc->player_slots[slot] = -1;
    }

    int lastIdx        = wc->cnt - 1;
    int lastId         = wc->dense[lastIdx];
    wc->players[idx]   = wc->players[lastIdx];
    wc->dense[idx]     = lastId;
    wc->sparse[lastId] = idx;
    wc->sparse[id]     = -1;
    wc->cnt--;
    WRemB(world, id, HAS_PLAYER);
}

int Sol_Player_GetEnt(World *world, int localIdx)
{
    WorldPlayers *wc = world->dense_components[WORLD_SYS_PLAYER];
    if (!wc)
        return -1;
    if (localIdx < 0 || localIdx > MAX_LOCAL_PLAYERS - 1)
        return -1;

    int entId = wc->player_slots[localIdx];
    // Verify entity is still alive and has component
    if (entId != -1 && WHasB(world, entId, HAS_PLAYER))
    {
        return entId;
    }
    return -1;
}