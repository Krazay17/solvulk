#pragma once
#include <stdatomic.h>

typedef struct
{
    atomic_bool isRunning;
} SolState;

extern SolState solState;

void Sol_Quit();