#pragma once
#include "types.h"

#define MAX_USER_ITEMS 512

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

int  Sol_User_Init(void);
void Sol_User_Tick(double dt);
void Sol_User_Draw(double dt);
void Sol_User_SaveUserSettings(int flags);
void Sol_User_LoadUserSettings(int flags);
void Sol_User_HydrateUI();