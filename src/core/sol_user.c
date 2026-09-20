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

SolUser sol_user = {.view_world = -1, .view_ent = -1, .menu_world = -1, .game_world = -1, .hud_world = -1};
static SolResource user_settings_file;

static const SolActions key_binds[SOL_KEY_COUNT] = {
    [SOL_KEY_Q] = ACTION_ABILITY1, [SOL_KEY_E] = ACTION_ABILITY2,

    [SOL_KEY_1] = ACTION_ABILITY3, [SOL_KEY_2] = ACTION_ABILITY4, [SOL_KEY_3] = ACTION_ABILITY5,
    [SOL_KEY_4] = ACTION_ABILITY6,

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

void Find_User_Hit(double dt, SolMouse mouse)
{
    bool click = mouse.buttons[SOL_MOUSE_LEFT];
    bool grab  = mouse.buttons[SOL_MOUSE_MIDDLE];

    sol_user.target   = 0;
    sol_user.target_w = -1;
    sol_user.grab     = 0;

    for (int i = 0; i < solState.worldCount; i++)
    {
        World *world = solState.worlds[i];
        if (world->doesSimulate)
        {
            int hit_ent = Sol_Interact_FindTopmost(world, Sol_Input_GetMouseUI());
            if (hit_ent > 0)
            {
                sol_user.target   = hit_ent;
                sol_user.target_w = world->index;
                break;
            }
        }
    }
    if (sol_user.focus > 0)
    {
        if (!click && !grab)
        {
            sol_user.focus   = 0;
            sol_user.focus_w = -1;
        }
        sol_user.grab = grab;
        // else if (glms_vec2_distance2(sol_user.mouse_pos, sol_user.focus_start) > drag_dist2)
        //     sol_user.grab = 1;
        return;
    }

    if (sol_user.target > 0 && (click || grab))
    {
        sol_user.focus       = sol_user.target;
        sol_user.focus_w     = sol_user.target_w;
        sol_user.focus_start = sol_user.mouse_pos;
        sol_user.grab        = grab;
    }
}

static void Sol_User_LoadUserSettings()
{
    Sol_ReadFile(USER_SETTINGS_FILENAME, &user_settings_file);
}

void Sol_User_SyncUI()
{
    World *hud = Sol_GetWorldByIdx(WORLDID_HUD);
    if (!hud)
        return;
    SparseSet_ScRef *ref_set   = Sol_Comp_Set(hud, ScRef);
    bool items[MAX_USER_ITEMS] = {0};
    for (int i = ref_set->cnt - 1; i >= 0; i--)
    {
        int id     = ref_set->dense[i];
        ScRef *ref = &ref_set->data[i];
        if (ref->kind != REFKIND_ITEM)
            continue;
        if (ref->index < 0 || ref->index >= user_data.itemCount)
        {
            Sol_Destroy_Ent(hud, id);
            continue;
        }
        items[ref->index] = true;
    }

    // Spawn new refs
    for (int i = 0; i < user_data.itemCount; i++)
    {
        if (!items[i])
        {
            SolItem *item = &user_data.items[i];
            float x       = 100.0f + (i % 5) * 64.0f;
            float y       = 100.0f + (i / 5) * 64.0f;
            Sol_Prefab_AbilityCard(hud, (vec3s){x, y, 0}, item->kind, i);
        }
    }

    World *game = Sol_User_GetGameWorld();
    if (!game)
        return;
    SparseSet_ScPlayer *player_set = Sol_Comp_Set(game, ScPlayer);
    bool player_healthbars[32]     = {0};
    bool player_abilitybars[32]    = {0};
    // Cleanup UI from destroyed player ents
    for (int i = ref_set->cnt - 1; i >= 0; i--)
    {
        ScRef *ref = &ref_set->data[i];
        if (ref->kind != REFKIND_HEALTHBAR && ref->kind != REFKIND_ABILITYBAR)
            continue;
        int ref_id     = ref_set->dense[i];
        int ent_id     = ref->ent_id;
        int player_idx = player_set->sparse[ent_id];
        bool is_alive  = (player_idx >= 0 && player_idx < player_set->cnt && player_set->dense[player_idx] == ent_id);
        if (!is_alive)
        {
            Sol_Destroy_Ent(hud, ref_id);
            continue;
        }
        if (player_idx < 32)
        {
            if (ref->kind == REFKIND_HEALTHBAR)
                player_healthbars[player_idx] = true;
            else if (ref->kind == REFKIND_ABILITYBAR)
                player_abilitybars[player_idx] = true;
        }
    }

    int view_id = sol_user.view_ent;
    // Spawn missing healthbars
    for (int p = 0; p < player_set->cnt; p++)
    {
        if (p < 32 && !player_healthbars[p])
        {
            int player_ent = player_set->dense[p];
            float x        = 0.0f;
            float y        = 50.0f + (p * 30.0f);
            if (player_ent == view_id)
            {
                x = (WINDOW_WIDTH * 0.5f) - (300.0f * 0.5f);
                y = 620.0f;
            }
            int healthbar_id = Sol_Prefab_Healthbar(hud, (vec3s){x, y, 0});

            *Sol_Comp_Add(hud, healthbar_id, ScRef) = (ScRef){
                .kind      = REFKIND_HEALTHBAR,
                .ent_world = game->index,
                .ent_id    = player_ent,
            };
        }
    }
    for (int p = 0; p < player_set->cnt; p++)
    {
        if (p < 32 && !player_abilitybars[p])
        {
            int player_ent = player_set->dense[p];
            float x        = 200.0f;
            float y        = 50.0f + (p * 50.0f);
            if (player_ent == view_id)
            {
                x = (WINDOW_WIDTH * 0.5f) - (434.0f * 0.5f);
                y = 700.0f;
            }
            int abilitybar_id = Sol_Prefab_AbilityBar(hud, (vec3s){x, y, 0}, 7);

            *Sol_Comp_Add(hud, abilitybar_id, ScRef) = (ScRef){
                .kind      = REFKIND_ABILITYBAR,
                .ent_world = game->index,
                .ent_id    = player_ent,
            };
        }
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
    Sol_User_LoadUserSettings();
    if (user_settings_file.data)
    {
        memcpy(&user_data, user_settings_file.data, sizeof(UserData));
    }
    else
        LoadDefaults();

    return 0;
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
            if (action != 0)
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
                Sol_Debug_AddText("AbilityState", ability_state_name[ability->state]);
                Sol_Debug_AddText("PrevAState", ability_state_name[prev_Astate]);
                prev_Astate = ability->state;
            }
        }
    }
}

void Sol_User_Tick(double dt)
{
    SolMouse mouse         = Sol_Input_GetMouse();
    sol_user.mouse_pos     = Sol_Input_GetMousePos();
    sol_user.mouse_pos_ui  = Sol_Input_GetMouseUI();
    sol_user.interact_last = sol_user.interact;
    sol_user.interact      = mouse.buttons[SOL_MOUSE_LEFT];
    sol_user.grab_last     = sol_user.drag;
    sol_user.grab          = mouse.buttons[SOL_MOUSE_MIDDLE];

    // sol_user.click_r = mouse.buttons[SOL_MOUSE_RIGHT];
    consume_mouse = false;
    consume_key   = false;

    Find_User_Hit(dt, mouse);
    Sol_User_SyncUI();

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
    World *game    = Sol_User_GetGameWorld();
    int id         = sol_user.view_ent;
    if (!game || id <= 0)
        return;
    ScCamera *cam = Sol_Comp_Get(game, id, ScCamera);
    if (cam)
    {
        g_solView.pos    = cam->pos;
        g_solView.roll   = cam->roll;
        g_solView.dir    = cam->dir;
        g_solView.fov    = cam->fov;
        g_solView.up     = cam->up;
        g_solView.target = vecAdd(cam->pos, cam->dir);
    }
    ScCombat *combat = Sol_Comp_Get(game, id, ScCombat);
    if (combat)
    {
        static float last_damage = 0;
        if (combat->damageDone != last_damage)
        {
            last_damage = combat->damageDone;
            Sol_Audio_Play(SOL_AUDIO_HIT, 0.2f, 0, 32);
        }
    }

    // World *hud               = Sol_GetWorldByIdx(WORLDID_HUD);
    // SparseSet_ScRef *ref_set = Sol_Comp_Set(hud, ScRef);
    // SlEvent *events          = Sol_Comp_Get(game, 0, SlEvent);
    // for (int i = 0; i < solb_count(events->events); i++)
    // {
    //     SolEvent event = events->events[i];
    //     if (event.kind != EVENTKIND_ENT_DESTROY)
    //         continue;
    //     for (int j = ref_set->cnt - 1; j >= 0; j--)
    //     {
    //         ScRef *ref = &ref_set->data[j];
    //         if (ref->ent_world == game->index && ref->ent_id == event.as.ent_destroy.id)
    //         {
    //             Sol_Destroy_Ent(hud, ref_set->dense[j]);
    //         }
    //     }
    // }

    // if (Sol_Comp_Has(world, id, ScCmd))
    // {
    //     vec3s head = Sol_Body3_GetHead(world, id);
    //     ScCmd *cmd = Sol_Comp_Get(world, id, ScCmd);
    //     if (mouse.buttons[SOL_MOUSE_LEFT])
    //     {
    //         SolRay ray = {.start = head, .dir = cmd->aimdir, .dist = 30.0f, .mask = (COLLAYER_WORLD |
    //         COLLAYER_TEAMZ)}; vec3s final_pos     = vecAdd(ray.start, vecSca(ray.dir, ray.dist)); SolRayResult result
    //         = {0}; bool hit            = Sol_Raycast1D(world, ray, &result, 0.1f); if (hit)
    //         {
    //             final_pos = result.pos;
    //         }

    //         SolSphere *sphere = Sol_Debug_NewSphere(world, 0.1f);
    //         sphere->color     = VEC4_GREEN;
    //         sphere->pos       = final_pos;
    //         sphere->radius    = 0.2f;
    //     }
    // }
}
void Sol_User_Draw(double dt)
{
    World *world = Sol_GetWorldByIdx(sol_user.target_w);
    int id       = sol_user.target;
    Sol_Tooltip_Draw(world, id, (float)dt);
}

void Sol_User_SaveUserSettings()
{
    Sol_WriteFile(USER_SETTINGS_FILENAME, &user_data, sizeof(UserData));
}

void Sol_User_ClearUserSettings()
{
    remove(USER_SETTINGS_FILENAME);
    LoadDefaults();
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
    user_data.items[user_data.itemCount++] = *item;
}

SolItem *Sol_User_GetItemAtIdx(int idx)
{
    return &user_data.items[idx];
}