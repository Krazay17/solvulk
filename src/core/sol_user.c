#include "sol_user.h"
#include "sol_core.h"
#include "sol_math.h"
#include "world.h"
#include "input.h"
#include "platform/platform.h"

#define USER_SETTINGS_FILENAME "UserData"

static SolResource user_settings_file;

static const SolActions key_binds[SOL_KEY_COUNT] = {
    [SOL_KEY_Q] = ACTION_ABILITY1, [SOL_KEY_E] = ACTION_ABILITY2,

    [SOL_KEY_1] = ACTION_ABILITY3, [SOL_KEY_2] = ACTION_ABILITY4,
    [SOL_KEY_3] = ACTION_ABILITY5, [SOL_KEY_4] = ACTION_ABILITY6,

    [SOL_KEY_5] = ACTION_ABILITY7, [SOL_KEY_6] = ACTION_ABILITY8,
    [SOL_KEY_7] = ACTION_ABILITY9, [SOL_KEY_W] = ACTION_FWD,
    [SOL_KEY_A] = ACTION_LEFT,     [SOL_KEY_S] = ACTION_BWD,
    [SOL_KEY_D] = ACTION_RIGHT,    [SOL_KEY_F] = 0,
    [SOL_KEY_SPACE] = ACTION_JUMP, [SOL_KEY_ESCAPE] = 0,
    [SOL_KEY_SHIFT] = ACTION_DASH, [SOL_KEY_CTRL] = ACTION_CROUCH,
};
static const SolActions mouse_binds[SOL_MOUSE_COUNT] = {
    [SOL_MOUSE_LEFT]  = ACTION_ABILITY1,
    [SOL_MOUSE_RIGHT] = ACTION_ABILITY2,
};

bool         consume_mouse;
bool         consume_key;
UserData     user_data = {.look_sens = 0.001f};
static float tooltipAlpha;

SolUserHit user_hit = {
    .hoverId    = -1,
    .focusId    = -1,
    .focusWorld = NULL,
    .hoverWorld = NULL,
    .isFocusUi  = false,
    .isHoverUi  = false,
    .isDragging = false,
};

void Find_User_Hit(void)
{
    // if (user_hit.hoverId != -1 && user_hit.hoverWorld)
    // {
    //     Sol_Interact_RemState(user_hit.hoverWorld, user_hit.hoverId, INTERACT_HOVERED);
    //     Sol_Interact_RemState(user_hit.hoverWorld, user_hit.hoverId, INTERACT_CLICKED);
    // }
    // user_hit.hoverId = -1;
    // for (int i = 0; i < solState.worldCount; i++)
    // {
    //     World *world = solState.worlds[i];
    //     if (!world || !world->doesSimulate)
    //         continue;
    //     int topmost = Sol_Interact_GetTopmost(world);
    //     if (topmost != -1)
    //     {
    //         user_hit.hoverId    = topmost;
    //         user_hit.hoverWorld = world;
    //         Sol_Interact_AddState(world, user_hit.hoverId, INTERACT_HOVERED);
    //         break;
    //     }
    // }

    // SolMouse mouse = Sol_Input_GetMouse();
    // if (user_hit.focusId != -1)
    // {
    //     if (user_hit.isDragging)
    //     {
    //         Sol_Interact_DragEntityTo(user_hit.focusWorld, user_hit.focusId,
    //                                   (vec3s){Sol_Input_GetMouseUI().x, Sol_Input_GetMouseUI().y});
    //         if (mouse.buttonsReleased[SOL_MOUSE_LEFT])
    //         {
    //             Sol_Interact_EndDrag(user_hit.focusWorld, user_hit.focusId);
    //             user_hit.focusId    = -1;
    //             user_hit.isDragging = false;
    //         }
    //     }
    //     else
    //     {
    //         if (glms_ivec2_distance2(user_hit.pressPos, (ivec2s){Sol_Input_GetMouse().x, Sol_Input_GetMouse().y}) >
    //             1.0f)
    //         {
    //             user_hit.isDragging = true;
    //         }
    //         if (mouse.buttonsReleased[SOL_MOUSE_LEFT])
    //         {
    //             Sol_Interact_RemState(user_hit.focusWorld, user_hit.focusId, INTERACT_PRESSED);
    //             Sol_Interact_AddState(user_hit.focusWorld, user_hit.focusId, INTERACT_CLICKED);
    //             user_hit.focusId = -1;
    //         }
    //         Sol_Interact_AddState(user_hit.focusWorld, user_hit.focusId, INTERACT_PRESSED);
    //     }
    // }
    // else if (user_hit.hoverId != -1)
    // {
    //     if (mouse.buttons[SOL_MOUSE_LEFT])
    //     {
    //         user_hit.focusId    = user_hit.hoverId;
    //         user_hit.focusWorld = user_hit.hoverWorld;
    //         user_hit.isFocusUi  = user_hit.isHoverUi;
    //         user_hit.pressPos   = (ivec2s){Sol_Input_GetMouse().x, Sol_Input_GetMouse().y};
    //     }
    // }
}

static void Sol_User_LoadUserSettings(int flags)
{
    Sol_ReadFile(USER_SETTINGS_FILENAME, &user_settings_file);
}

void Sol_User_HydrateUI()
{
    World *world = Sol_GetWorldById(WORLDID_HUD);
    if (!world)
        return;
    for (int i = 0; i < user_data.itemCount; i++)
    {
        SolItem *item = &user_data.items[i];
        float    x    = 100.0f + (i % 5) * 64.0f;
        float    y    = 100.0f + (i / 5) * 64.0f;
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

void Tooltip_Update(double dt, SolUserHit user_hit)
{
    const float stiffness = 5.0f;
    float       alpha     = 1.0f - expf(-stiffness * dt);
    if (user_hit.hoverId && user_hit.hoverWorld)
    {
        int    id    = user_hit.hoverId;
        World *world = user_hit.hoverWorld;
        if (Sol_Comp_Has(world, id, SolTooltip))
        {
            //            tooltipAlpha = Sol_Math_Lerp(tooltipAlpha, MAX_TOOLTIP_ALPHA, alpha);
            return;
        }
    }
    tooltipAlpha = 0;
}

void Sol_User_Tick(double dt, double time)
{
    consume_mouse = false;
    consume_key   = false;

    Find_User_Hit();
    Tooltip_Update(dt, user_hit);

    Sol_User_HydrateUI();

    if (Sol_Input_KeyPressed(SOL_KEY_ESCAPE))
    {
        bool menuActive = solState.worlds[0]->doesSimulate;
        menuActive ^= 1;
        solState.worlds[0]->doesSimulate = menuActive;
        solState.worlds[0]->doesRender   = menuActive;
        Sol_Input_SetLocked(!menuActive);
    }

    // ### DEBUG ####
    // World *activeWorld = Sol_GetActiveGameWorld();
    // int    playerId    = Sol_Player_GetEnt(activeWorld, 0);
    // if (playerId > -1)
    // {
    //     SolXform *xform = Sol_Comp_Get(activeWorld, playerId, SolXform);
    //     if (xform)
    //     {
    //         Sol_Debug_Add("X", xform->pos.x);
    //         Sol_Debug_Add("Y", xform->pos.y);
    //         Sol_Debug_Add("Z", xform->pos.z);
    //         // float speed = glms_vec3_norm(Sol_Physx_GetVel(activeWorld, playerId));
    //         // Sol_Debug_Add("Velocity", speed);
    //         Sol_Debug_Add("State", Sol_Comp_Get(activeWorld, playerId, SolMovement)->state);
    //     }
    // }
}

void Sol_User_Draw(double dt, double time)
{
    // Sol_Tooltip_Draw(user_hit, tooltipAlpha, dt, time);
}

void Sol_User_SaveUserSettings(int flags)
{
    user_settings_file.data = &user_data;
    user_settings_file.size = sizeof(UserData);
    Sol_WriteFile(USER_SETTINGS_FILENAME, &user_settings_file);
}