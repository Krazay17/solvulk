#include "loader.h"

static SolBank bank = {0};

void LoadModels()
{
    SolModel cpu = SolLoadModel("ID_MODEL_WIZARD");
    SolModelHandle handle = Sol_UploadModel(&cpu);
    bank.models.wizard = handle;
    SolFreeModel(&cpu);
}

SolBank *GetBank(void)
{
    return &bank;
}
