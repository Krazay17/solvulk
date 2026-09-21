#include "sol/types.h"
#include "estate.h"

int Find_Target(World *world, int id, ScAi *ai, ScCmd *cmd, int team);
void Fill_Brain(World *world, int id, ScAi *ai, ScCmd *cmd, float fdt);
void Fill_Knows(World *world, int id, ScAi *ai, ScCmd *cmd);
void Fill_Reward(World *world, int id, ScAi *ai, float fdt);
void Submit_Learn(World *world, int id, ScAi *ai, ScCmd *cmd);
void Convert_AiActions(ScAi *ai, ScCmd *cmd);