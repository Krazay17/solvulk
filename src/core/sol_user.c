#include "sol_user.h"
#include "sol_engine.h"
#include "sol_math.h"
#include "world.h"
#include "input.h"
#include "platform/platform.h"
#include "interact/s_interact.h"

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

void Sol_User_Init(void)
{
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