#include <stdio.h>

#include "sol_ui.h"
#include "world.h"
#include "input.h"

#include "render.h"

static SolButton *movingButton = NULL;
static float dragOffsetX;
static float dragOffsetY;

static float FlashAnim(float dt, float value, float speed);
static float PulseAnim(float dt, float value, float speed);
static void Sol_Button_Move();

void Sol_System_Button_Update(World *world, double dt, double time)
{
    uint32_t required = HAS_INTERACT | HAS_SHAPE | HAS_XFORM;
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) == required)
        {
            CompInteractable *interact = &world->interactables[id];
            if (interact->isClicked)
                printf("Clicked Button %d\n", id);
        }
    }
}

void Sol_Button_Update(SolButton *buttons, int offset, int count, float dt)
{
    if (!buttons)
        return;
    SolMouse mouse = SolInput_GetMouse();

    for (int i = offset; i < offset + count; ++i)
    {
        ButtonState prev = buttons[i].state;
        ButtonState next = 0;

        bool hovered = Sol_Check_2d_Collision((vec2){mouse.x, mouse.y}, buttons[i].rect);
        if (prev & BUTTON_MOVING && mouse.buttons[SOL_MOUSE_MIDDLE])
            next |= BUTTON_MOVING;
        if (hovered)
        {
            next |= BUTTON_HOVERED;
            if (mouse.buttonsPressed[SOL_MOUSE_LEFT])
                next |= BUTTON_PRESSED;
            if (mouse.buttons[SOL_MOUSE_LEFT])
                next |= BUTTON_HELD;
            if (mouse.buttons[SOL_MOUSE_MIDDLE] && !movingButton)
            {
                movingButton = &buttons[i];
                dragOffsetX = mouse.x - buttons[i].rect.x;
                dragOffsetY = mouse.y - buttons[i].rect.y;
                next |= BUTTON_MOVING;
                Sol_Button_ToFront(buttons, i, offset, count);
            }
        }

        buttons[i].state = next;

        float clickTarget = 0.0f;
        float hoverTarget = hovered ? 1.0f : 0.0f;
        buttons[i].hoverAnim += (hoverTarget - buttons[i].hoverAnim) * (float)dt * 20.0f;
        buttons[i].clickAnim += (clickTarget - buttons[i].clickAnim) * (float)dt * 4.0f;

        if (next & BUTTON_PRESSED)
        {
            buttons[i].clickAnim = 1.0f;
            if (buttons[i].callback)
                buttons[i].callback(i, buttons[i].userData);
        }
    }
    Sol_Button_Move();
}

void Sol_Button_Draw(SolButton *buttons, int offset, int count)
{
    if (!buttons)
        return;
    for (int j = 0; j < offset + count; ++j)
    {
        int i = buttons[j].drawOrder;
        SolColor base = buttons[i].color;
        float h = buttons[i].hoverAnim;
        float c = buttons[i].clickAnim;

        // brighten on hover, flash white on click
        SolColor col = {
            .r = (uint8_t)fminf(base.r + h * 100 + c * 100, 255),
            .g = (uint8_t)fminf(base.g + h * 100 + c * 100, 255),
            .b = (uint8_t)fminf(base.b + h * 100 + c * 100, 255),
            .a = base.a,
        };
        Sol_Draw_Rectangle(buttons[i].rect, col, 0);
        if (buttons[i].state & BUTTON_HOVERED)
        {
            Sol_Draw_Rectangle(buttons[i].rect, (SolColor){1, 1, 1, 255}, 1.0f + c * 3.0f);
        }
        if (!buttons[i].text)
            continue;
        float fontSize = buttons[i].fontSize;
        float textX = buttons[i].rect.x + (buttons[i].rect.w * 0.5f) - (buttons[i].textWidth * 0.5f);
        float textY = buttons[i].rect.y + (buttons[i].rect.h / 2.0f) + (fontSize / 3.0f);
        Sol_Draw_Text(buttons[i].text, textX, textY, fontSize, buttons[i].textColor);
    }
}

static void Sol_Button_Move()
{
    SolMouse mouse = SolInput_GetMouse();
    if (!mouse.buttons[SOL_MOUSE_MIDDLE])
        movingButton = NULL;
    if (movingButton)
    {
        movingButton->rect.x = mouse.x - dragOffsetX;
        movingButton->rect.y = mouse.y - dragOffsetY;
    }
}

void Sol_Button_Reset(SolButton *buttons)
{
}

void Sol_Button_ToFront(SolButton *buttons, int i, int offset, int count)
{
    int pos = -1;
    for (int j = 0; j < offset + count; j++)
        if (buttons[j].drawOrder == i)
        {
            pos = j;
            break;
        }

    if (pos != -1)
    {
        for (int j = pos; j < offset + count - 1; j++)
            buttons[j].drawOrder = buttons[j + 1].drawOrder;
        buttons[count - 1].drawOrder = i;
    }
}

void Sol_Button_InitText(SolButton *button, SolColor color, const char *text, float fontSize)
{
    button->textColor = color;
    const char *buttonText = text;
    button->fontSize = fontSize;
    button->text = buttonText;
    button->textWidth = Sol_MeasureText(buttonText, fontSize);
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