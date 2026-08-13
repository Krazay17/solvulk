#pragma once
#include "types.h"

typedef struct UserSettings
{
    float look_sens;
} UserSettings;

extern UserSettings user_settings;

void Sol_User_Tick(double dt);
void Sol_User_Draw(double dt);