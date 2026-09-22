#include "sol/types.h"
#include "estate.h"

static inline u32 Ai_GetStateIndex(const AiStateInputs *inputs)
{
    u32 index = 0;
    u32 stride = 1;

    // 1. Mutually Exclusive Enums
    index += inputs->dist * stride;
    stride *= AITARGETDIST_COUNT;

    index += inputs->motion * stride;
    stride *= AIMOTION_COUNT;

    index += inputs->height * stride;
    stride *= AIHEIGHT_COUNT;

    index += inputs->targetCombat * stride;
    stride *= AITARGET_COUNT;

    index += inputs->self * stride;
    stride *= AISELF_COUNT;

    index += inputs->knows * stride;

    return index;
}

int Find_Target(World *world, int id, ScAi *ai, ScCmd *cmd, int team);
void Fill_Brain(World *world, int id, ScAi *ai, ScCmd *cmd, float fdt);
void Fill_Knows(World *world, int id, ScAi *ai, ScCmd *cmd);
void Fill_Reward(World *world, int id, ScAi *ai, float fdt);
void Submit_Learn(World *world, int id, ScAi *ai, ScCmd *cmd);
void Convert_AiActions(ScAi *ai, ScCmd *cmd);
