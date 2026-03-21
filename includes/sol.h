#pragma once
#include <stdbool.h>
#include <stdatomic.h>

typedef struct
{
    float windowWidth, windowHeight;
} SolState;

extern SolState solState;

void Sol_Quit();