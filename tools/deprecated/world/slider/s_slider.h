#pragma once
#include "sol/types.h"

typedef struct CompSlider
{
    float value, min, max;
    float handle_width, handle_height;
} CompSlider;

void        Sol_Slider_Init(World *world);
CompSlider *Sol_Slider_Add(World *world, int id);
CompSlider *Sol_Slider_Get(World *world, int id);
bool        Sol_Slider_Has(World *world, int id);
void        Sol_Slider_Rem(World *world, int id);