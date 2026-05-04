#pragma once
#include "sol/base.h"

typedef struct CompXform
{
    vec3s   pos, lastPos, drawPos;
    versors quat, lastQuat, drawQuat;
    vec3s   scale, lastScale, drawScale;
    float   width, height;
} CompXform;