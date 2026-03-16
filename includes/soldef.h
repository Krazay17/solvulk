#pragma once
#include <stdbool.h>
#include "solmath.h"

#define MAX_BUTTONS 100

typedef enum
{
    BUTTON_ISHOVERED = 1,
    BUTTON_ISPRESSED = 1 << 1,
    BUTTON_WASHOVERED = 1 << 2,
    BUTTON_WASPRESSED = 1 << 3,
} ButtonState;

typedef struct
{
    SolRect rect[MAX_BUTTONS];
    SolColor color[MAX_BUTTONS];
    const char *text[MAX_BUTTONS];
    float clickAnim[MAX_BUTTONS];
    float hoverAnim[MAX_BUTTONS];
    ButtonState state[MAX_BUTTONS];
} SolButton;