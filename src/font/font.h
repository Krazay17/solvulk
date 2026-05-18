#pragma once
#include "sol/types.h"

typedef enum
{
    SOL_FONT_ICE,
    SOL_FONT_COUNT,
} SolFontKind;

typedef struct
{
    const char *str;
    float       x, y, size;
    vec4s       color;
    SolFontKind kind;
} SolFontDesc;

float           Sol_MeasureText(const char *str, float size, SolFontKind id);