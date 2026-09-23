#include "sol/types.h"
#include "estate.h"

#define AI_ALPHA 0.2f
#define AI_GAMMA 0.96f
#define AI_EXPLORE 0.2f

int Find_Target(World *world, int id, ScAi *ai, ScCmd *cmd, int team);
void Fill_Brain(World *world, int id, ScAi *ai, ScCmd *cmd, float fdt);
void Fill_Reward(World *world, int id, ScAi *ai, float fdt);
AiKnowState Get_Knows(World *world, int id, ScAi *ai, ScCmd *cmd);
void Submit_Learn(World *world, int id, ScAi *ai, ScCmd *cmd);
void Convert_AiActions(ScAi *ai, ScCmd *cmd);