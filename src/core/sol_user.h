#pragma once
#include "types.h"

#define MAX_USER_ITEMS 512

typedef struct SolUser
{
    int        menu_world, game_world, hud_world;
    int        view_ent;
    float      yaw, pitch;
    SolActions actions;
    int        mouse_hover_ent, mouse_drag_ent;
    bool       mouse_locked;
} SolUser;

typedef struct UserData
{
    float      look_sens;
    float      volume;
    SolActions key_binds[SOL_KEY_COUNT];
    SolActions mouse_binds[SOL_MOUSE_COUNT];

    SolItem items[MAX_USER_ITEMS];
    int     itemCount;
} UserData;

extern UserData user_data;
extern SolUser  sol_user;

int    Sol_User_Init(void);
void   Sol_User_Tick(double dt);
void   Sol_User_PostTick(double dt);
void   Sol_User_Draw(double dt);
void   Sol_User_SaveUserSettings(int flags);
void   Sol_User_LoadUserSettings(int flags);
void   Sol_User_HydrateUI();
World *Sol_User_GetGameWorld();