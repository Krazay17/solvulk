#pragma once
#include "solmath.h"

#define MAX_BUTTONS 200

typedef void (*SolButtonCallback)(int index, void* userData);

typedef enum
{
    BUTTON_HOVERED = 1 << 0,
    BUTTON_PRESSED = 1 << 1,
    BUTTON_HELD    = 1 << 2,
    BUTTON_MOVING  = 1 << 3,
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
    SolButtonCallback callback[MAX_BUTTONS];
    void* userData[MAX_BUTTONS]; // To pass things like "World*" or "GameState*"
} SolButton;

void Sol_Button_Update(SolButton *button, int offset, int count, float dt);
void Sol_Button_Draw(SolButton *button);