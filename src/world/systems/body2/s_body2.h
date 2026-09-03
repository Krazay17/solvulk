#pragma once
#include "sol/types.h"

#define TERMINAL_VELOCITY 1000.0f

// TODO does Body2 need spatial grid/hash?
typedef struct
{
    SolContact *contacts;
} SysBody2;
