/*
 * File: sol_user.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-04
 *
 */
#include "sol_user.h"
#include "sol_core.h"
#include "sol_math.h"
#include "world.h"
#include "input.h"
#include "platform/platform.h"
#include "render/render.h"

#include "ability/s_ability.h"

#define USER_SETTINGS_FILENAME "UserData"

SolUser sol_user = {.active_world = -1, .menu_world = -1, .game_world = -1, .hud_world = -1, .view_ent = -1};
static SolResource user_settings_file;

static const SolActions key_binds[SOL_KEY_COUNT] = {
    [SOL_KEY_Q] = ACTION_ABILITY1, [SOL_KEY_E] = ACTION_ABILITY2,

    [SOL_KEY_1] = ACTION_ABILITY3, [SOL_KEY_2] = ACTION_ABILITY4, [SOL_KEY_3] = ACTION_ABILITY5,
    [SOL_KEY_4] = ACTION_ABILITY6,

    [SOL_KEY_5] = ACTION_ABILITY7, [SOL_KEY_6] = ACTION_ABILITY8, [SOL_KEY_7] = ACTION_ABILITY9,
    [SOL_KEY_W] = ACTION_FWD,      [SOL_KEY_A] = ACTION_LEFT,     [SOL_KEY_S] = ACTION_BWD,
    [SOL_KEY_D] = ACTION_RIGHT,    [SOL_KEY_F] = ACTION_INTERACT, [SOL_KEY_SPACE] = ACTION_JUMP,
    [SOL_KEY_ESCAPE] = 0,          [SOL_KEY_SHIFT] = ACTION_DASH, [SOL_KEY_CTRL] = ACTION_CROUCH,
    [SOL_KEY_TAB] = ACTION_SCORE,
};
static const SolActions mouse_binds[SOL_MOUSE_COUNT] = {
    [SOL_MOUSE_LEFT]  = ACTION_ABILITY1,
    [SOL_MOUSE_RIGHT] = ACTION_ABILITY2,
};

bool consume_mouse;
bool consume_key;
UserData user_data = {.look_sens = 0.001f};
static float tooltipAlpha;

void Find_User_Hit(void)
{
    SolMouse mouse          = Sol_Input_GetMouse();
    vec2s mouse_pos         = Sol_Input_GetMouseUI();
    sol_user.mouse_interact = mouse.buttons[SOL_MOUSE_LEFT];
    sol_user.mouse_pos      = mouse_pos;

    if (sol_user.mouse_focus_ent >= 0)
    {
        if (!sol_user.mouse_interact)
        {
            sol_user.mouse_hover_ent      = sol_user.mouse_focus_ent;
            sol_user.mouse_hover_worldidx = sol_user.mouse_focus_worldidx;
            sol_user.mouse_focus_ent      = -1;
            sol_user.mouse_focus_worldidx = -1;
        }
        else
        {
            ScInteract *interact =
                Sol_Comp_Get(Sol_GetWorldByIdx(sol_user.mouse_focus_worldidx), sol_user.mouse_focus_ent, ScInteract);
            if (interact)
            {
                interact->drag_target = sol_user.mouse_pos;
            }
            return;
        }
    }

    sol_user.mouse_hover_worldidx = -1;
    sol_user.mouse_hover_ent      = -1;

    for (int i = 0; i < solState.worldCount; i++)
    {
        World *world = solState.worlds[i];
        if (world->doesSimulate)
        {
            int hit_ent = Sol_Interact_FindTopmost(world, mouse_pos);
            if (hit_ent >= 0)
            {
                sol_user.mouse_hover_ent      = hit_ent;
                sol_user.mouse_hover_worldidx = world->index;
                break;
            }
        }
    }
    if (sol_user.mouse_hover_ent >= 0 && sol_user.mouse_interact)
    {
        sol_user.mouse_focus_ent      = sol_user.mouse_hover_ent;
        sol_user.mouse_focus_worldidx = sol_user.mouse_hover_worldidx;
    }
}

static void Sol_User_LoadUserSettings(int flags)
{
    Sol_ReadFile(USER_SETTINGS_FILENAME, &user_settings_file);
}

void Sol_User_HydrateUI()
{
    World *world = Sol_GetWorldByIdx(WORLDID_HUD);
    if (!world)
        return;
    for (int i = 0; i < user_data.itemCount; i++)
    {
        SolItem *item = &user_data.items[i];
        float x       = 100.0f + (i % 5) * 64.0f;
        float y       = 100.0f + (i / 5) * 64.0f;
        //       Sol_Prefab_AbilityCard(world, (vec3s){x, y, 0}, item->ability, item->rarity);
    }
}

static void LoadDefaults(void)
{
    user_data = (UserData){
        .look_sens = 0.001f,
    };
    memcpy(user_data.key_binds, key_binds, sizeof(SolActions) * SOL_KEY_COUNT);
    memcpy(user_data.mouse_binds, mouse_binds, sizeof(SolActions) * SOL_MOUSE_COUNT);
}

int Sol_User_Init(void)
{
    Sol_User_LoadUserSettings(0);
    if (user_settings_file.data)
    {
        memcpy(&user_data, user_settings_file.data, sizeof(UserData));
    }
    else
        LoadDefaults();

    return 0;
}

void Tooltip_Update(double dt)
{
    const float stiffness = 5.0f;
    float alpha           = 1.0f - expf(-stiffness * dt);
    int id                = sol_user.mouse_hover_ent;
    int worldidx          = sol_user.mouse_hover_worldidx;
    World *world          = Sol_GetWorldByIdx(worldidx);
    if (world && id >= 0)
    {
        if (Sol_Comp_Has(world, id, ScTooltip))
        {
            //            tooltipAlpha = Sol_Math_Lerp(tooltipAlpha, MAX_TOOLTIP_ALPHA, alpha);
            return;
        }
    }
    tooltipAlpha = 0;
}

void Entity_Actions()
{
    World *world = Sol_User_GetGameWorld();
    int id       = sol_user.view_ent;
    if (!world || id < 0)
        return;
    ScCamera *camera = Sol_Comp_Get(world, id, ScCamera);
    ScCmd *cmd       = Sol_Comp_Get(world, id, ScCmd);
    SolMouse mouse   = Sol_Input_GetMouse();

    sol_user.mouse_locked = mouse.locked;
    sol_user.actions      = 0;
    float *yaw            = &sol_user.yaw;
    float *pitch          = &sol_user.pitch;

    if (mouse.locked)
    {
        *yaw -= (float)(mouse.dx * user_data.look_sens);
        *yaw -= (2.0f * GLM_PIf) * floorf((*yaw + GLM_PIf) / (2.0f * GLM_PIf));

        *pitch -= (float)(mouse.dy * user_data.look_sens);
        *pitch = glm_clamp(*pitch, -MAX_PITCH, MAX_PITCH);
    }

    for (int i = 0; i < SOL_KEY_COUNT; i++)
    {
        if (Sol_Input_KeyDown(i))
        {
            SolActions action = user_data.key_binds[i];
            if (action != ACTION_NONE)
                sol_user.actions |= BITC(action);
        }
    }

    if (Sol_Comp_Has(world, id, ScBuilder))
    {
        if (mouse.buttons[SOL_MOUSE_LEFT])
            sol_user.actions |= BITC(ACTION_BUILD);
    }
    else
    {
        if (mouse.togglelocked)
        {
            if (mouse.buttons[SOL_MOUSE_LEFT])
                sol_user.actions |= BITC(user_data.mouse_binds[SOL_MOUSE_LEFT]);

            if (mouse.buttons[SOL_MOUSE_RIGHT])
                sol_user.actions |= BITC(user_data.mouse_binds[SOL_MOUSE_RIGHT]);
        }
        else if (mouse.locked && mouse.buttons[SOL_MOUSE_LEFT])
            sol_user.actions |= BITC(ACTION_FWD);

        if (mouse.wheelV && camera)
        {
            float changeDist = -((float)mouse.wheelV * 0.01f);
            camera->desired_distance += changeDist;
        }
    }

    // DEBUG FLY
    if (Sol_Input_KeyDown(SOL_KEY_G))
    {
        Xform xform = Xform_Get(world, id);
        vec3s pos   = vecAdd(xform.pos, vecSca(vecNorm(Sol_Vec3_FromYawPitch(sol_user.yaw, sol_user.pitch)), 0.1f));
        Sol_Xform_Teleport(world, id, pos);
        if (Sol_Comp_Has(world, id, ScBody3))
        {
            ScBody3 *body3 = Sol_Comp_Get(world, id, ScBody3);
            body3->vel     = GLMS_VEC3_ZERO;
        }
    }
}

const char *move_state_name[MOVE_STATE_COUNT] = {
    [MOVE_IDLE] = "Idle",         [MOVE_WALK] = "Walk",     [MOVE_STUN] = "Stun",       [MOVE_FALL] = "Fall",
    [MOVE_JUMP] = "Jump",         [MOVE_CROUCH] = "Crouch", [MOVE_SLIDE] = "Slide",     [MOVE_WALLRUN] = "Wallrun",
    [MOVE_WALLJUMP] = "Walljump", [MOVE_MANTLE] = "Mantle", [MOVE_LANDING] = "Landing", [MOVE_FLY] = "Fly",
    [MOVE_DEAD] = "Dead",
};

void User_Debug(dt)
{
    World *world = Sol_User_GetGameWorld();
    if (!world)
        return;
    int id = sol_user.view_ent;
    if (id >= 0)
    {
        Xform xform = Xform_Get(world, id);
        Sol_Debug_Add("X", xform.pos.x);
        Sol_Debug_Add("Y", xform.pos.y);
        Sol_Debug_Add("Z", xform.pos.z);
        if (Sol_Comp_Has(world, id, ScMove3))
        {
            ScMove3 *move = Sol_Comp_Get(world, id, ScMove3);

            static u32 prev_state = 0;

            bool state_changed = move->state != prev_state;
            if (state_changed)
            {
                Sol_Debug_AddText("MoveState", move_state_name[move->state]);
                Sol_Debug_AddText("PrevState", move_state_name[prev_state]);
                prev_state = move->state;
            }
        }
        if (Sol_Comp_Has(world, id, ScAbility))
        {
            ScAbility *ability = Sol_Comp_Get(world, id, ScAbility);

            static u32 prev_Astate = 0;

            bool state_changed = ability->state != prev_Astate;
            if (state_changed)
            {
                Sol_Debug_AddText("AbilityState", ability_names[ability->state]);
                Sol_Debug_AddText("PrevAState", ability_names[prev_Astate]);
                prev_Astate = ability->state;
            }
        }
    }
}

void Sol_User_Tick(double dt)
{
    consume_mouse = false;
    consume_key   = false;

    Find_User_Hit();
    Tooltip_Update(dt);

    Sol_User_HydrateUI();

    if (Sol_Input_KeyPressed(SOL_KEY_ESCAPE))
    {
        bool menuActive = solState.worlds[0]->doesSimulate;
        menuActive ^= 1;
        solState.worlds[0]->doesSimulate = menuActive;
        solState.worlds[0]->doesRender   = menuActive;
        Sol_Input_SetLocked(!menuActive);
    }

    Entity_Actions();
    User_Debug();
}

void Sol_User_PostTick(double dt)
{
    SolMouse mouse = Sol_Input_GetMouse();
    World *world   = Sol_User_GetGameWorld();
    int id         = sol_user.view_ent;
    if (!world || id < 0)
        return;
    ScCamera *cam = Sol_Comp_Get(world, id, ScCamera);
    if (cam)
    {
        g_solView.pos    = cam->pos;
        g_solView.roll   = cam->roll;
        g_solView.dir    = cam->dir;
        g_solView.fov    = cam->fov;
        g_solView.up     = cam->up;
        g_solView.target = vecAdd(cam->pos, cam->dir);
    }

    if (Sol_Comp_Has(world, id, ScCmd))
    {
        ScCmd *cmd = Sol_Comp_Get(world, id, ScCmd);
        if (mouse.buttons[SOL_MOUSE_LEFT])
        {
            SolRay ray = {
                .start = cmd->headpos, .dir = cmd->aimdir, .dist = 30.0f, .mask = (COLLAYER_WORLD | COLLAYER_TEAMZ)};
            vec3s final_pos     = vecAdd(ray.start, vecSca(ray.dir, ray.dist));
            SolRayResult result = {0};
            bool hit            = Sol_Raycast1D(world, ray, &result, 1.0f);
            if (hit)
            {
                final_pos = result.pos;
            }

            SolSphere *sphere = Sol_Debug_NewSphere(world, 5.0f);
            sphere->color     = VEC4_GREEN;
            sphere->pos       = final_pos;
            sphere->radius    = 1.0f;
        }
    }
}

void Sol_User_Draw(double dt)
{
    // Sol_Tooltip_Draw(user_hit, tooltipAlpha, dt, time);
}

void Sol_User_SaveUserSettings(int flags)
{
    Sol_WriteFile(USER_SETTINGS_FILENAME, &user_data, sizeof(UserData));
}

World *Sol_User_GetGameWorld()
{
    if (sol_user.game_world < 0)
        return NULL;
    return solState.worlds[sol_user.game_world];
}

void Sol_User_EnterGameWorld(u32 idx)
{
    if (solState.worldCount == 0)
        return;
    if (sol_user.game_world == idx || solState.worldCount - 1 < idx)
        return;

    u32 last_world_idx = sol_user.game_world;
    World *last_world  = solState.worlds[last_world_idx];

    last_world->doesSimulate = false;
    last_world->doesRender   = false;

    sol_user.game_world        = idx;
    World *target_world        = solState.worlds[sol_user.game_world];
    target_world->doesSimulate = true;
    target_world->doesRender   = true;

    SparseSet_ScPlayer *set = Sol_Comp_Set(target_world, ScPlayer);
    sol_user.view_ent       = set->dense[0];
}