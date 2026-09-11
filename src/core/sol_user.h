#pragma once
#include "types.h"

#define MAX_USER_ITEMS 512

typedef enum
{
    USERACTION_LCLICK = (1 << 0),
    USERACTION_RCLICK = (1 << 1),
    USERACTION_MCLICK = (1 << 2),
    USERACTION_MENU   = (1 << 3),
} UserAction;
typedef struct SolUser
{
    int view_world;
    int view_ent;
    int menu_world, game_world, hud_world;
    float yaw, pitch;
    SolActions actions;
    bool mouse_locked;
    vec2s mouse_pos, focus_start;
    bool interact;
    bool interact_last;
    bool grab, grab_last;

    int target, target_w;
    int focus, focus_w;
    int drag, drag_w;
} SolUser;

typedef struct UserData
{
    float look_sens;
    float volume;
    SolActions key_binds[SOL_KEY_COUNT];
    SolActions mouse_binds[SOL_MOUSE_COUNT];

    SolItem items[MAX_USER_ITEMS];
    int itemCount;
} UserData;

extern UserData user_data;
extern SolUser sol_user;

int Sol_User_Init(void);
void Sol_User_Tick(double dt);
void Sol_User_PostTick(double dt);
void Sol_User_Draw(double dt);
void Sol_User_SaveUserSettings(int flags);
void Sol_User_LoadUserSettings(int flags);
void Sol_User_HydrateUI();
World *Sol_User_GetGameWorld();
void Sol_User_EnterGameWorld(u32 idx, bool sim_last, vec3s pos);