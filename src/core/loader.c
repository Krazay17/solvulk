#include "loader.h"

static SolBank bank = {0};

void LoadModels()
{
    SolModel cpu = SolLoadModel("ID_MODEL_WIZARD");
    SolGpuModel gpu = Sol_UploadModel(&cpu);
    bank.models.gpuWizard = gpu;
    SolFreeModel(&cpu);
}

SolBank *GetBank(void)
{
    return &bank;
}
