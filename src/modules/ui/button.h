#pragma once
#include "solmath.h"

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
    SolRect rect;
    SolColor color;
    SolColor textColor;
    const char *text;
    float fontSize;
    float textWidth;
    float clickAnim;
    float hoverAnim;
    ButtonState state;
    ButtonAction action;
    int drawOrder;
    SolButtonCallback callback;
    void* userData;
} SolButton;

void Sol_Button_Update(SolButton *buttons, int offset, int count, float dt);
void Sol_Button_Draw(SolButton *buttons, int offset, int count);
void Sol_Button_ToFront(SolButton *buttons,int i, int offset, int count);
void Sol_Button_InitText(SolButton *button, SolColor color, const char *text, float fontSize);