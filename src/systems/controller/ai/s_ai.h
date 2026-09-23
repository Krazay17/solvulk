#include "sol/types.h"
#include "estate.h"

#define AI_ALPHA 0.3f
#define AI_GAMMA 0.96f
#define AI_EXPLORE 0.3f

int Find_Target(World *world, int id, ScAi *ai, ScCmd *cmd, int team);
void Fill_Brain(World *world, int id, ScAi *ai, ScCmd *cmd, float fdt);
void Fill_Reward(World *world, int id, ScAi *ai, float fdt);
AiKnowStateM Get_KnowsM(World *world, int id, ScAi *ai, ScCmd *cmd);
void Submit_Learn(World *world, int id, ScAi *ai, ScCmd *cmd);
void Convert_AiActions(ScAi *ai, ScCmd *cmd, AiKnowStateM next_knows_move, AiKnowStateC next_knows_combat);
void Learn_Table(float *table, u32 num_actions, u32 next_knows, u32 action, u32 knows, float reward, float alpha,
                 float gamma);
