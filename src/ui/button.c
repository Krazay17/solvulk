#include "soldef.h"
#include "input.h"
#include "render.h"

static float FlashAnim(float dt, float value, float speed);
static float PulseAnim(float dt, float value, float speed);

void Sol_Button_Update(SolButton *button, int offset, int count, float dt)
{
    SolMouse mouse = SolInput_GetMouse();
    for (int i = 0; i < MAX_BUTTONS; ++i)
    {
        if (button->clickAnim[i] > 0)
            button->clickAnim[i] = FlashAnim(dt, button->clickAnim[i], 0);
            
        bool hovered = Sol_Check_2d_Collision((vec2){mouse.x, mouse.y}, button->rect[i]);
        if (hovered)
        {
            button->state[i] |= BUTTON_ISHOVERED;
            if (mouse.buttonsPressed[SOL_MOUSE_LEFT])
            {
                button->state[i] |= BUTTON_ISPRESSED;
                button->clickAnim[i] = 1;
                printf("Clicked Button %d\n", i);
            }
        }
    }
}

void Sol_Button_Draw(SolButton *button)
{
    for(int i = 0; i < MAX_BUTTONS; ++i)
    {
        Sol_Draw_Rectangle(button->rect[i], button->color[i]);
    }
}

void Sol_Button_Reset(SolButton *button)
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