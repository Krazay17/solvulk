#pragma once

#ifdef SOL_DEV_MODE
#include "sol_core.h"
#else
#include "sol/sol.h"
#endif

void Create_Sol_Game();

void MakeAWizard(void *data);

void QuitApp(void *data);
void W_Set_Ontop(void *data);