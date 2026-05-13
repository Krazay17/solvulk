#pragma once
#include "sol/types.h"

typedef enum
{
    UI_BUTTON,
    UI_SLIDER,
    UI_COUNT,
} UiKind;
typedef struct
{
    UiKind      kind;
    vec4s       baseColor;
    vec4s       textColor;
    float       fontSize;
    const char *text;
    float       textWidth;

    float hoverAnim;
    float clickAnim;

    float borderThickness;
    vec4s borderColor;
} UiDesc;

void Sol_Ui_Init(World *world);
void Sol_Ui_Add(World *world, int id, UiDesc desc);
void Sol_Ui_Draw(World *world, double dt, double time);
