#include "solmath.h"

bool Sol_Check_2d_Collision(vec2 a, SolRect b)
{
    return !((a[0] < b.x) |
             (a[0] >= b.x + b.w) |
             (a[1] < b.y) |
             (a[1] >= b.y + b.h));
}

float Sol_Lerp(float start, float end, float amount)
{
    return start + amount * (end - start);
}