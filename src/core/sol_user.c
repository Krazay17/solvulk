/*
 * File: sol_user.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-04
 *
 */
#include "sol_user.h"
#include "world.h"
#include "input.h"
#include "sol_core.h"
#include "sol_math.h"
#include "platform/platform.h"
#include "render/render.h"
#include "audio.h"
#include "prefabs.h"

#define USER_SETTINGS_FILENAME "UserData"

UserCore sol_user  = {.view_world = -1, .view_ent = -1, .menu_world = -1, .game_world = -1, .hud_world = -1};
UserData user_data = {.user_config.look_sens = 0.001f};

static float tooltip_alpha;
static SolResource user_settings_file;

static const UserActions user_keys[SOL_KEY_COUNT] = {
    [SOL_KEY_ESCAPE] = USERACTION_ESCAPE,
    [SOL_KEY_C]      = USERACTION_INVENTORY,
};
static const UserActions user_mouse[SOL_MOUSE_COUNT] = {
    [SOL_MOUSE_LEFT]   = USERACTION_MOUSEINTERACT,
    [SOL_MOUSE_RIGHT]  = USERACTION_MOUSEMENU,
    [SOL_MOUSE_MIDDLE] = USERACTION_MOUSEGRAB,
};

static const SolActions key_binds[SOL_KEY_COUNT] = {
    [SOL_KEY_1]     = ACTION_ABILITY1, //
    [SOL_KEY_2]     = ACTION_ABILITY2, //
    [SOL_KEY_3]     = ACTION_ABILITY3, //
    [SOL_KEY_4]     = ACTION_ABILITY4, //
    [SOL_KEY_SHIFT] = ACTION_ABILITY5, //
    [SOL_KEY_Q]     = ACTION_ABILITY6, //
    [SOL_KEY_E]     = ACTION_ABILITY7, //

    [SOL_KEY_W]     = ACTION_FWD,      //
    [SOL_KEY_A]     = ACTION_LEFT,     //
    [SOL_KEY_S]     = ACTION_BWD,      //
    [SOL_KEY_D]     = ACTION_RIGHT,    //
    [SOL_KEY_F]     = ACTION_INTERACT, //
    [SOL_KEY_SPACE] = ACTION_JUMP,     //
    [SOL_KEY_CTRL]  = ACTION_CROUCH,   //
    [SOL_KEY_TAB]   = ACTION_SCORE,    //
};
static const SolActions mouse_binds[SOL_MOUSE_COUNT] = {
    [SOL_MOUSE_LEFT]  = ACTION_ABILITY6,
    [SOL_MOUSE_RIGHT] = ACTION_ABILITY7,
};

void UserHit_Update(UserHit *user_hit)
{
    bool down              = sol_user.user_input.user_actions & (1u << USERACTION_MOUSEINTERACT);
    bool grab              = sol_user.user_input.user_actions & (1u << USERACTION_MOUSEGRAB);
    user_hit->target       = 0;
    user_hit->target_world = NULL;
    for (int i = 0; i < solState.worldCount; i++)
    {
        World *world = solState.worlds[i];
        if (world->doesSimulate)
        {
            int hit_ent = Sol_Interact_FindTopmost(world, sol_user.user_input.mouse);
            if (hit_ent > 0)
            {
                user_hit->target       = hit_ent;
                user_hit->target_world = world;
                break;
            }
        }
    }

    if (user_hit->grab > 0 && !grab)
    {
        user_hit->grab       = 0;
        user_hit->grab_world = NULL;
    }
    if (user_hit->focus > 0 && !down)
    {
        user_hit->focus       = 0;
        user_hit->focus_world = NULL;
    }

    if (!user_hit->grab && !user_hit->focus && user_hit->target > 0 && (down || grab))
    {
        user_hit->focus       = user_hit->target;
        user_hit->focus_world = user_hit->target_world;
        if (grab)
        {
            user_hit->grab       = user_hit->target;
            user_hit->grab_world = user_hit->target_world;
            Sol_Interact_BeginDrag(user_hit->grab_world, user_hit->grab,
                                   (vec3s){sol_user.user_input.mouse.x, sol_user.user_input.mouse.y});
        }
    }

    if (user_hit->grab)
        Sol_Interact_Drag(user_hit->grab_world, user_hit->grab,
                          (vec3s){sol_user.user_input.mouse.x, sol_user.user_input.mouse.y});
    else if (user_hit->focus)
        Sol_Interact_Down(user_hit->focus_world, user_hit->focus,
                          (vec3s){sol_user.user_input.mouse.x, sol_user.user_input.mouse.y});
    else if (user_hit->target)
        Sol_Interact_Post(user_hit->target_world, user_hit->target, (1u << INTERACT_HOVERED));
}

void Sol_User_LoadUserSettings()
{
    user_settings_file = Sol_LoadResource("data/", USER_SETTINGS_FILENAME);
}

static void LoadDefaults(void)
{
    memset(&user_data, 0, sizeof(UserData));
    user_data.user_config.look_sens = 0.001f;
    // solb_init(user_data.inventory.items, 8);
    memcpy(user_data.user_config.key_binds, key_binds, sizeof(SolActions) * SOL_KEY_COUNT);
    memcpy(user_data.user_config.mouse_binds, mouse_binds, sizeof(SolActions) * SOL_MOUSE_COUNT);
}

int Sol_User_Init(void)
{
    Sol_User_LoadUserSettings();
    if (user_settings_file.data)
    {
        memcpy(&user_data, user_settings_file.data, sizeof(UserData));
    }
    else
        LoadDefaults();

    return 0;
}

void Sol_User_SaveUserSettings()
{
    size_t size = sizeof(UserData);
    // size += solb_count(user_data.inventory.items) * sizeof(SolItem);
    Sol_WriteFile("data/", USER_SETTINGS_FILENAME, &user_data, size);
}

void Sol_User_ClearUserSettings()
{
    Sol_DeleteFile("data/", USER_SETTINGS_FILENAME);
    LoadDefaults();
}

void Entity_Actions()
{
    World *world = Sol_User_GetGameWorld();
    int id       = sol_user.view_ent;
    if (!world || id < 0)
        return;
    SolMouse mouse = Sol_Input_GetMouse();

    sol_user.actions = 0;
    float *yaw       = &sol_user.yaw;
    float *pitch     = &sol_user.pitch;

    if (mouse.locked)
    {
        *yaw -= (float)(mouse.dx * user_data.user_config.look_sens);
        *yaw -= (2.0f * GLM_PIf) * floorf((*yaw + GLM_PIf) / (2.0f * GLM_PIf));

        *pitch -= (float)(mouse.dy * user_data.user_config.look_sens);
        *pitch = glm_clamp(*pitch, -MAX_PITCH, MAX_PITCH);
    }

    for (int i = 0; i < SOL_KEY_COUNT; i++)
    {
        if (Sol_Input_KeyDown(i))
        {
            SolActions action = user_data.user_config.key_binds[i];
            if (action != 0)
                sol_user.actions |= 1u << action;
        }
    }

    if (mouse.togglelocked)
    {
        if (mouse.buttons[SOL_MOUSE_LEFT])
            sol_user.actions |= BITC(user_data.user_config.mouse_binds[SOL_MOUSE_LEFT]);

        if (mouse.buttons[SOL_MOUSE_RIGHT])
            sol_user.actions |= BITC(user_data.user_config.mouse_binds[SOL_MOUSE_RIGHT]);
    }
    else if (mouse.locked && mouse.buttons[SOL_MOUSE_LEFT])
        sol_user.actions |= BITC(ACTION_FWD);

    forc(world, id, ScCamera)
    {
        if (mouse.wheelV)
        {
            float changeDist = -((float)mouse.wheelV / 240.0f);
            c->desired_distance += changeDist;
        }
    }

    SparseSet_ScPlayer *player_set = Sol_Comp_Set(world, ScPlayer);
    for (int i = 0; i < player_set->cnt; i++)
    {
        int player_id = player_set->dense[i];
        ScPlayer *c   = &player_set->data[i];
        c->actions    = sol_user.actions;
        if (mouse.locked)
        {
            c->strafing = true;
            c->yaw -= (float)(mouse.dx * user_data.user_config.look_sens);
            c->yaw -= (2.0f * GLM_PIf) * floorf((c->yaw + GLM_PIf) / (2.0f * GLM_PIf));

            c->pitch -= (float)(mouse.dy * user_data.user_config.look_sens);
        }
        else
            c->strafing = false;

        c->pitch = glm_clamp(c->pitch, -MAX_PITCH, MAX_PITCH);
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

void User_Debug(double dt)
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
    }
}

const float show_healthbar_d2 = 100.0f;
static void ShowHealthbar_OnDistance(World *world, int user_ent)
{
    if (!world || !user_ent)
        return;
    vec3s pos_user          = world->xform.pos[user_ent];
    SparseSet_ScCombat *set = Sol_Comp_Set(world, ScCombat);
    for (int i = 0; i < set->cnt; i++)
    {
        int id = set->dense[i];
        if (id == user_ent)
            continue;
        vec3s pos   = world->xform.pos[id];
        vec3s delta = vecSub(pos, pos_user);
        float d2    = vecDot(delta, delta);
        if (d2 <= show_healthbar_d2)
        {
            forc(world, id, ScView3)
            {
                for (int v = 0; v < solb_count(c->views_b); v++)
                {
                    View3 *view = &c->views_b[v];
                    if (view->kind == VIEW3KIND_POWERBAR)
                    {
                        view->_elapsed = 0;
                    }
                }
            }
        }
    }
}

void User_Input()
{
    sol_user.user_input.mouse        = Sol_Input_GetMouseUI();
    sol_user.user_input.mouse_locked = Sol_Input_MouseLocked();
    sol_user.user_input.prev_actions = sol_user.user_input.user_actions;
    sol_user.user_input.user_actions = 0;
    sol_user.user_input.just_actions = 0;
    for (int i = 0; i < SOL_KEY_COUNT; i++)
    {
        if (Sol_Input_KeyDown(i))
        {
            UserActions action = user_keys[i];
            if (action != 0)
                sol_user.user_input.user_actions |= 1u << action;
        }
    }
    for (int i = 0; i < SOL_MOUSE_COUNT; i++)
    {
        if (Sol_Input_MouseDown(i))
        {
            UserActions action = user_mouse[i];
            if (action != 0)
            {
                sol_user.user_input.user_actions |= 1u << action;
            }
        }
    }
    for (int i = 0; i < USERACTION_COUNT; i++)
    {
        if (sol_user.user_input.user_actions & 1u << i && !(sol_user.user_input.prev_actions & 1u << i))
            sol_user.user_input.just_actions |= 1u << i;
    }
}

void Sol_User_Tick(double dt)
{
    User_Input();
    UserHit_Update(&sol_user.user_hit);
    World *menu = Sol_GetWorldByIdx(sol_user.menu_world);
    World *game = Sol_GetWorldByIdx(sol_user.game_world);
    World *hud  = Sol_GetWorldByIdx(sol_user.hud_world);
    int id      = sol_user.view_ent;

    if (hud)
    {
        Sol_Ui_User_Update(hud, game, id);
        forc(game, id, ScInventory)
        {
            if (c->dirty_equip)
                Sol_Ui_Sync_AbilityItems(hud, game, id);
            if (c->dirty && sol_user.inventory.open)
                Sol_Ui_Sync_Inventory(hud, game, id, sol_user.inventory.pos);
        }
    }

    ShowHealthbar_OnDistance(game, id);

    if (menu && Sol_Input_KeyPressed(SOL_KEY_ESCAPE))
    {
        bool menuActive = solState.worlds[0]->doesSimulate;
        menuActive ^= 1;
        solState.worlds[0]->doesSimulate = menuActive;
        solState.worlds[0]->doesRender   = menuActive;
        Sol_Input_SetLocked(!menuActive);
    }

    if (game && Sol_Input_KeyPressed(SOL_KEY_5))
    {
        if (Sol_Buff_HasBuff(game, id, BUFFKIND_INVULN))
            Sol_Buff_Rem(game, id, BUFFKIND_INVULN);
        else
        {
            Sol_Buff_Next(game, id, BUFFKIND_INVULN)->inf = true;
        }
    }

    if (hud)
    {
        if (sol_user.user_input.user_actions & (1u << USERACTION_INVENTORY))
        {
            if (!sol_user.inventory.open)
            {
                Sol_Ui_Sync_Inventory(hud, game, id, sol_user.inventory.pos);
                sol_user.inventory.open = true;
            }
            Sol_Input_SetLocked(false);
        }
        else if (sol_user.inventory.open)
        {
            Sol_Ui_CloseInv(hud, &sol_user.inventory.pos);
            sol_user.inventory.open = false;
            Sol_Input_SetLocked(true);
        }
    }

    Entity_Actions();
    User_Debug(dt);
}

void Sol_User_PostTick(double dt)
{
    World *game = Sol_User_GetGameWorld();
    int id      = sol_user.view_ent;
    if (!game || id <= 0)
        return;

    forc(game, id, ScCamera)
    {
        g_solView.pos    = c->pos;
        g_solView.roll   = c->roll;
        g_solView.dir    = c->dir;
        g_solView.fov    = c->fov;
        g_solView.up     = c->up;
        g_solView.target = vecAdd(c->pos, c->dir);
    }

    forc(game, id, ScCombat)
    {
        static float last_damage = 0;
        if (c->damageDone != last_damage)
        {
            last_damage = c->damageDone;
            Sol_Audio_Play(SOL_AUDIO_HIT, 0.2f, 0, 32);
        }
    }
}
void Sol_User_Draw(double dt)
{
    float fdt    = (float)dt;
    World *world = sol_user.user_hit.target_world;
    int id       = sol_user.user_hit.target;
    if (world && Sol_Comp_Has(world, id, ScTooltip))
    {
        tooltip_alpha = Sol_Math_Lerp(tooltip_alpha, MAX_TOOLTIP_ALPHA, 1.0f - expf(-5.0f * fdt));
        Sol_Tooltip_Draw(world, id, &sol_user.user_input, tooltip_alpha);
    }
    else
        tooltip_alpha = 0;
}

World *Sol_User_GetGameWorld()
{
    if (sol_user.game_world < 0)
        return NULL;
    return solState.worlds[sol_user.game_world];
}

void Sol_User_EnterGameWorld(u32 idx, bool sim_last, vec3s pos)
{
    if (solState.worldCount == 0 || sol_user.game_world == idx || solState.worldCount - 1 < idx)
        return;

    u32 last_world_idx = sol_user.game_world;
    World *last_world  = solState.worlds[last_world_idx];

    last_world->doesSimulate = sim_last;
    last_world->doesRender   = false;

    sol_user.game_world        = idx;
    World *target_world        = solState.worlds[sol_user.game_world];
    target_world->doesSimulate = true;
    target_world->doesRender   = true;
    SparseSet_ScPlayer *set    = Sol_Comp_Set(last_world, ScPlayer);
    int last_id                = set->dense[0];
    sol_user.view_ent          = Sol_Duplicate_Ent(last_world, last_id, target_world, pos);
    Sol_Destroy_Ent(last_world, last_id);
}
void Sol_User_AddItem(SolItem *item)
{
    solb_push(user_data.inventory.items, *item);
}
void Sol_User_AddItemLoose(SolItem *item)
{
    user_data.loose_items[user_data.loose_item_count++] = *item;
}

SolItem *Sol_User_GetItemAtIdx(int idx)
{
    return &user_data.loose_items[idx];
}
