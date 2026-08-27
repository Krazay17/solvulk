#pragma once
#include "sol/types.h"

typedef struct
{
    u8    substeps;
    float sub_dt;
} SubstepData;

SubstepData Substep_Get(float speed, float radius, float fdt);