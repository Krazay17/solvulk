#include "sol_user.h"
#include "sol_engine.h"
#include "sol_math.h"
#include "world.h"
#include "input.h"
#include "platform/platform.h"
#include "camera.h"

#include "xform/s_xform.h"
#include "interact/s_interact.h"
#include "controller/s_controller.h"

#include "buff/s_buff.h"

#define USER_SETTINGS_FILENAME "UserSettings"

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
UserSettings user_settings = {.look_sens = 0.001f};
SolUserHit   user_hit      = {
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
    if (user_hit.hoverId != -1 && user_hit.hoverWorld)
    {
        Sol_Interact_ClearState(user_hit.hoverWorld, user_hit.hoverId, INTERACT_HOVERED);
        Sol_Interact_ClearState(user_hit.hoverWorld, user_hit.hoverId, INTERACT_CLICKED);
    }
    user_hit.hoverId = -1;
    for (int i = 0; i < solEngine.worldCount; i++)
    {
        World *world = solEngine.worlds[i];
        if (!world || !world->doesSimulate)
            continue;
        int topmost = Sol_Interact_GetTopmost(world);
        if (topmost != -1)
        {
            user_hit.hoverId    = topmost;
            user_hit.hoverWorld = world;
            user_hit.isHoverUi  = world->kind == WORLDKIND_MENU ? true : false;
            Sol_Interact_AddState(world, user_hit.hoverId, INTERACT_HOVERED);
            break;
        }
    }

    SolMouse mouse = Sol_Input_GetMouse();
    if (user_hit.focusId != -1)
    {
        if (user_hit.isDragging)
        {
            Sol_Interact_DragEntityTo(user_hit.focusWorld, user_hit.focusId,
                                      (vec3s){Sol_Input_GetMouseUI().x, Sol_Input_GetMouseUI().y});
            if (mouse.buttonsReleased[SOL_MOUSE_LEFT])
            {
                Sol_Interact_EndDrag(user_hit.focusWorld, user_hit.focusId);
                user_hit.focusId    = -1;
                user_hit.isDragging = false;
            }
        }
        else
        {
            if (glms_ivec2_distance2(user_hit.pressPos, (ivec2s){Sol_Input_GetMouse().x, Sol_Input_GetMouse().y}) >
                1.0f)
            {
                user_hit.isDragging = true;
            }
            if (mouse.buttonsReleased[SOL_MOUSE_LEFT])
            {
                Sol_Interact_AddState(user_hit.focusWorld, user_hit.focusId, INTERACT_CLICKED);
                user_hit.focusId = -1;
            }
        }
    }
    else if (user_hit.hoverId != -1)
    {
        if (mouse.buttonsPressed[SOL_MOUSE_LEFT])
        {
            user_hit.focusId    = user_hit.hoverId;
            user_hit.focusWorld = user_hit.hoverWorld;
            user_hit.isFocusUi  = user_hit.isHoverUi;
            user_hit.pressPos   = (ivec2s){Sol_Input_GetMouse().x, Sol_Input_GetMouse().y};
        }
    }
}

void UserControllerUpdate(World *world, double dt, double time)
{
    float           fdt        = (float)dt;
    int             localId    = 1;
    CompController *controller = &world->controllers[localId];
    if (!WHasB(world, localId, HAS_CONTROLLER))
        return;
    SolMouse mouse = Sol_Input_GetMouse();

    float *yaw   = &controller->yaw;
    float *pitch = &controller->pitch;

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
        else
            controller->actionState &= ~user_settings.key_binds[i];
    }

    controller->isStrafing = mouse.locked;

    if (WHas(world, 1, BITC(HAS_BUILDING)))
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
            float changeDist = (float)mouse.wheelV * 0.01f;
            Sol_Camera_AdjustDistance(&solCamera, changeDist);
        }
    }
    controller->yaw     = *yaw;
    controller->pitch   = *pitch;
    controller->lookdir = Sol_Vec3_FromYawPitch(*yaw, *pitch);
    if (WHasB(world, localId, HAS_BODY3))
        Sol_Controller_SetParallaxAim(world, localId, Sol_Cam_GetPos(), controller->lookdir, 60.0f, 0.5f);

    // #### DEBUG ACTIONS ####
    if (Sol_Input_KeyDown(SOL_KEY_F))
    {
        controller->actionState |= ACTION_DEBUGTELE;
    }
    else
        controller->actionState &= ~ACTION_DEBUGTELE;

    if (Sol_Input_KeyPressed(SOL_KEY_5))
    {
        vec3s pos = Sol_Xform_GetPos(world, localId);
        if (!Sol_Buff_HasBuff(world, localId, BUFFKIND_INVULN))
            Sol_Buff_AddEx(world, localId, localId, BUFFKIND_INVULN, 99999.9f, 0);
        else
            Sol_Buff_Remove(world, localId, BUFFKIND_INVULN);
    }
    if (Sol_Input_KeyPressed(SOL_KEY_6))
    {
    }
}

void Sol_User_Worlds_Tick(World **worlds, int count, double dt, double time)
{
    for (int i = 0; i < count; i++)
    {
        World *world = worlds[i];
        if (!world || !world->doesSimulate)
            continue;
        UserControllerUpdate(world, dt, time);
    }
}

static void Sol_User_LoadUserSettings(void)
{
    Sol_ReadFile(USER_SETTINGS_FILENAME, &user_settings_file);
}

static void LoadDefaults(void)
{
    user_settings = (UserSettings){
        .look_sens = 0.001f,
    };
    memcpy(user_settings.key_binds, key_binds, sizeof(SolActions) * SOL_KEY_COUNT);
    memcpy(user_settings.mouse_binds, mouse_binds, sizeof(SolActions) * SOL_MOUSE_COUNT);
}

int Sol_User_Init(void)
{
    Sol_User_LoadUserSettings();
    if (user_settings_file.data)
    {
        memcpy(&user_settings, user_settings_file.data, sizeof(UserSettings));
    }
    else
        LoadDefaults();

    return 0;
}

void Sol_User_Tick(double dt)
{
    consume_mouse = false;
    consume_key   = false;

    Find_User_Hit();
    Sol_Tooltip_Update(dt, user_hit);

    if (Sol_Input_KeyPressed(SOL_KEY_ESCAPE))
    {
        bool menuActive = solEngine.worlds[0]->doesSimulate;
        menuActive ^= 1;
        solEngine.worlds[0]->doesSimulate = menuActive;
        solEngine.worlds[0]->doesRender   = menuActive;
        Sol_Input_SetLocked(!menuActive);
    }
}

void Sol_User_Draw(double dt)
{
    Sol_Tooltip_Draw(dt, user_hit);
}

void Sol_User_SaveUserSettings(void)
{
    user_settings_file.data = &user_settings;
    user_settings_file.size = sizeof(UserSettings);
    Sol_WriteFile(USER_SETTINGS_FILENAME, &user_settings_file);
}