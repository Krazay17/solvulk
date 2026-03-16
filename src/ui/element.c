#include "element.h"
#include <math.h>
#include "soldef.h"
#include "solmath.h"

static float FlashAnim(float dt, float value, float speed);
static float PulseAnim(float dt, float value, float speed);

void Sol_Element_Update(SolElement *element, float dt)
{
    if (element->clickAnim > 0)
        element->clickAnim = FlashAnim(dt, element->clickAnim, element->clickAnimSpeed);
}

void Sol_Element_Draw(SolElement *element)
{
}

void Sol_Element_Reset(SolElement *element)
{
}

static float FlashAnim(float dt, float value, float speed)
{
    speed = (speed > 0.0f) ? speed : 4.0f;
    value -= dt * speed;
    if (value < 0.0f)
        value = 0.0f;
    return value;
}

static float PulseAnim(float dt, float value, float speed)
{
    speed = (speed > 0.0f) ? speed : 4.0f;
    value += dt * speed;
    value = (sinf(value) * 0.5f) + 0.5f;
    return value;
}