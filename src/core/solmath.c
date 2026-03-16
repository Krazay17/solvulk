#include "solmath.h"

bool Sol_Check_2d_Collision(Vec2 a, Rect b)
{
    return !((a.x < b.x) |
             (a.x >= b.x + b.w) |
             (a.y < b.y) |
             (a.y >= b.y + b.h));
}

float Lerp(float start, float end, float amount)
{
    return start + amount * (end - start);
}