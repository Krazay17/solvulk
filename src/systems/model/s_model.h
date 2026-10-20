#pragma once
#include "sol/types.h"

extern void (*const model_kind_draw[MODELKIND_COUNT])(World *world, double dt, SolModel *model);