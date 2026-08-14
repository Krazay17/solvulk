#pragma once
#include "types.h"

typedef struct UserSettings
{
    float look_sens;
    float volume;
} UserSettings;

extern UserSettings user_settings;

void Sol_User_Tick(double dt);
void Sol_User_Draw(double dt);
void Sol_User_Worlds_Tick(World **worlds, int count, double dt, double time);