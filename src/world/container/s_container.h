#pragma once
#include "base.h"
#include "item/s_item.h"

typedef struct CompContainer
{
    u32   cnt, cap;
    Item *items;
} CompContainer;

void Sol_Container_Init(World *world);
void Sol_Container_Add(World *world, int id);