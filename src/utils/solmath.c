#include "solmath.h"

bool Sol_Check_2d_Collision(vec2 a, SolRect b)
{
    return !((a[0] < b.x) |
             (a[0] >= b.x + b.w) |
             (a[1] < b.y) |
             (a[1] >= b.y + b.h));
}

SolVec3 Sol_Vec_Add(vec3 a, vec3 b)
{
    return (SolVec3){a[0] + b[0],
                     a[1] + b[1],
                     a[2] + b[2]};
}

float Sol_Lerp(float start, float end, float amount)
{
    return start + amount * (end - start);
}