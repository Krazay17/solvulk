#pragma once

#ifdef SOL_DEV_MODE
#include "sol_core.h"
#else
#include "sol/sol.h"
#endif

void Create_Sol_Game();

void QuitApp(int flags, void *data);
void W_Set_Ontop(int flags, void *data);
void W_Set_Fullscreen(int flags, void *data);
