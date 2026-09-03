#pragma once
#include "types.h"

#define MAX_USER_ITEMS 512

typedef struct SolUser
{
    int        user_world;
    int        user_entid;
    float      yaw, pitch;
    SolActions actions;
    bool       isStrafing;
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

int  Sol_User_Init(void);
void Sol_User_Tick(double dt);
void Sol_User_PostTick(double dt);
void Sol_User_Draw(double dt);
void Sol_User_SaveUserSettings(int flags);
void Sol_User_LoadUserSettings(int flags);
void Sol_User_HydrateUI();