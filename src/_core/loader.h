#pragma once

#include "sol_bank.h"

#include "model/model.h"
#include "font/font.h"

extern const char *fontResourceName[SOL_FONT_COUNT][2];

void        Sol_Load_Resources();
SolResource Sol_LoadResource(const char *resourceName);
int         Sol_ReadFile(const char *filename, SolResource *outRes);
