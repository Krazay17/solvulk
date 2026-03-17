#pragma once
#include <stdbool.h>
#include "solmath.h"

#define MAX_BUTTONS 100

typedef enum
{
    BUTTON_IDLE       = 0,
    BUTTON_HOVERED    = 1 << 0,
    BUTTON_PRESSED    = 1 << 1,
    BUTTON_HELD         = 1 << 2,
    BUTTON_MOVING       = 1 << 3,
    BUTTON_JUST_ENTERED = 1 << 4,  // first frame mouse entered
    BUTTON_JUST_LEFT    = 1 << 5,  // first frame mouse left
    BUTTON_JUST_CLICKED = 1 << 6,  // first frame clicked
} ButtonState;

typedef enum
{
    BUTTON_ACTION_NONE,
    BUTTON_ACTION_QUIT,
} ButtonAction;

typedef struct
{
    SolRect rect[MAX_BUTTONS];
    SolColor color[MAX_BUTTONS];
    SolColor textColor[MAX_BUTTONS];
    const char *text[MAX_BUTTONS];
    float clickAnim[MAX_BUTTONS];
    float hoverAnim[MAX_BUTTONS];
    ButtonState state[MAX_BUTTONS];
    ButtonAction action[MAX_BUTTONS];
    int drawOrder[MAX_BUTTONS];
} SolButton;