#include "resource.h"

SolBank bank = {0};

SolBank *Sol_Bank_Get()
{
    return &bank;
}

SolModel *Sol_GetModel(SolModelId id)
{
    return &bank.models[id];
}