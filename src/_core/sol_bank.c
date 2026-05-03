#include "sol_bank.h"

SolBank bank = {0};

SolBank *Sol_Getbank()
{
    return &bank;
}

SolModel *Sol_GetModel(SolModelId id)
{
    return &bank.models[id];
}