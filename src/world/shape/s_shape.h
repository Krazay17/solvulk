#pragma once
#include "base.h"

typedef struct World World;

typedef enum
{
    SHAPEKIND_SPHERE,
    SHAPEKIND_FIREBALL,
} ShapeKind;
typedef struct CompShape
{
    ShapeKind kind;
    float     radius;
    vec4s     color;
} CompShape;

typedef struct
{
    ShapeKind kind;
    float     radius;
    vec4s     color;
} ShapeDesc;
void Sol_Shape_Init(World *world);
void Sol_Shape_Add(World *world, int id, ShapeDesc desc);
void Sol_Shape_Draw(World *world, double dt, double time);
void Sol_Shape_ColorAll(World *world, vec4s color);
