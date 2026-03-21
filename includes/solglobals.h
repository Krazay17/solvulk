#pragma once
#include <stdbool.h>
#include <stdatomic.h>

typedef struct
{
    bool isMovingButton;
} SolState;

extern SolState solState;

void Sol_Quit();