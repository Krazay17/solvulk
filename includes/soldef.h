#pragma once
#include <stdbool.h>

typedef struct
{
    float x, y, w, h;
} SolRect;

typedef struct
{
    float r, g, b, a;
} SolColor;

typedef struct
{
    SolRect rect;
    SolColor color;
    const char *text;
    float clickAnim;
    float hoverAnim;
    float clickAnimSpeed;
    float hoverAnimSpeed;
    bool isHovered;
    bool wasHovered;
    bool isPressed;
    bool wasPressed;
} SolElement;