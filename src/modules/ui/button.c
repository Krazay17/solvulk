#include "systems.h"
#include <stdio.h>

#include "input.h"
#include "render.h"

#define HELD_THRESHOLD 0.3f

static int movingButton = -1;
static float heldTime = 0.0f;

static float FlashAnim(float dt, float value, float speed);
static float PulseAnim(float dt, float value, float speed);

void Sol_Button_Update(SolButton *button, int offset, int count, float dt)
{
    SolMouse mouse = SolInput_GetMouse();

    bool interacted = false;

    for (int i = offset; i < offset + count; ++i)
    {
        ButtonState prev = button->state[i];
        ButtonState next = 0;

        bool hovered = Sol_Check_2d_Collision((vec2){mouse.x, mouse.y}, button->rect[i]);

        if (hovered)
        {
            next |= BUTTON_HOVERED;
            if (mouse.buttonsPressed[SOL_MOUSE_LEFT])
                next |= BUTTON_PRESSED;
            if (mouse.buttons[SOL_MOUSE_LEFT])
                next |= BUTTON_HELD;
            if (mouse.buttons[SOL_MOUSE_MIDDLE] && movingButton == -1)
            {
                movingButton = i;
                next |= BUTTON_MOVING;
            }
        }

        button->state[i] = next;

        float clickTarget = 0.0f;
        float hoverTarget = hovered ? 1.0f : 0.0f;
        button->hoverAnim[i] += (hoverTarget - button->hoverAnim[i]) * (float)dt * 20.0f;
        button->clickAnim[i] += (clickTarget - button->clickAnim[i]) * (float)dt * 4.0f;

        if (next & BUTTON_PRESSED)
        {
            button->clickAnim[i] = 1.0f;
            if (button->callback[i])
                button->callback[i](i, button->userData);
        }
        if (movingButton == i)
        {
            button->rect[i].x = mouse.x - (button->rect[i].w / 2.0f);
            button->rect[i].y = mouse.y - (button->rect[i].h / 2.0f);

            if (button->state[i] & ~BUTTON_MOVING)
            {
                button->state[i] |= BUTTON_MOVING;
                int pos = -1;
                for (int j = 0; j < MAX_BUTTONS; j++)
                    if (button->drawOrder[j] == i)
                    {
                        pos = j;
                        break;
                    }

                if (pos != -1)
                {
                    for (int j = pos; j < MAX_BUTTONS - 1; j++)
                        button->drawOrder[j] = button->drawOrder[j + 1];
                    button->drawOrder[MAX_BUTTONS - 1] = i;
                }
            }
        }
    }

    if(!mouse.buttons[SOL_MOUSE_MIDDLE])
        movingButton = -1;
}

void Sol_Button_Draw(SolButton *button)
{
    for (int j = 0; j < MAX_BUTTONS; ++j)
    {
        int i = button->drawOrder[j];
        SolColor base = button->color[i];
        float h = button->hoverAnim[i];
        float c = button->clickAnim[i];

        // brighten on hover, flash white on click
        SolColor col = {
            .r = (uint8_t)fminf(base.r + h * 100 + c * 100, 255),
            .g = (uint8_t)fminf(base.g + h * 100 + c * 100, 255),
            .b = (uint8_t)fminf(base.b + h * 100 + c * 100, 255),
            .a = base.a,
        };
        Sol_Draw_Rectangle(button->rect[i], col, 0);
        if (button->state[i] & BUTTON_HOVERED)
        {
            Sol_Draw_Rectangle(button->rect[i], (SolColor){1, 1, 1, 255}, 1.0f + c * 3.0f);
        }

        float fontSize = 16.0f;
        float textX = button->rect[i].x + (button->rect[i].w - Sol_MeasureText(button->text[i], fontSize)) / 2.0f;
        float textY = button->rect[i].y + (button->rect[i].h / 2.0f) + (fontSize / 3.0f);
        Sol_Draw_Text(button->text[i], textX, textY, fontSize, button->textColor[i]);
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