#pragma once
#include "types.h"

typedef struct UserSettings
{
    float look_sens;
    float volume;
    SolActions key_binds[SOL_KEY_COUNT];
    SolActions mouse_binds[SOL_MOUSE_COUNT];
} UserSettings;

extern UserSettings user_settings;

int Sol_User_Init(void);
void Sol_User_Tick(double dt);
void Sol_User_Draw(double dt);
void Sol_User_Worlds_Tick(World **worlds, int count, double dt, double time);
void Sol_User_SaveUserSettings(void);
void Sol_User_LoadUserSettings(void);