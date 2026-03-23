#include "app.h"

#include "ui/sol_ui.h"

#define BUTTON_COUNT 100

static void Init();
static void Tick(double dt, double time);
static void Draw();

static void ButtonPressed();

SolButton buttons[BUTTON_COUNT] = {0};
static GameState state = {0};
World menu = {
    .init = Init,
    .tick = Tick,
    .draw = Draw,
    .state = &state,
};

World *Sol_GetMenu(void)
{
    return &menu;
}

static void Init()
{
    for (int i = 0; i < BUTTON_COUNT; ++i)
    {
        int cols = 12;
        int col = i % cols;
        int row = i / cols;
        float startX = 100.0f;
        float startY = 100.0f;
        float xOffset = col * 110 + startX;
        float yOffset = row * 60 + startY;
        buttons[i].rect = (SolRect){xOffset, yOffset, 100, 50};
        buttons[i].color = (SolColor){255, 0, 0, 55};
        buttons[i].drawOrder = i;

        switch (i)
        {
        case 0:
            Sol_Button_InitText(&buttons[i], (SolColor){0, 255, 255, 55}, "QUIT", 16.0f);
            buttons[i].callback = Sol_Shutdown;
            break;
        case 1:
            Sol_Button_InitText(&buttons[i], (SolColor){0, 255, 255, 55}, "Colin", 16.0f);
            buttons[i].color = (SolColor){111, 255, 0, 255};
            break;
        default:
            Sol_Button_InitText(&buttons[i], (SolColor){0, 255, 255, 55}, "Button", 16.0f);
            buttons[i].action = BUTTON_ACTION_NONE;
        }
    }
}

static void Tick(double dt, double time)
{
    Sol_Button_Update(buttons, 0, BUTTON_COUNT, dt);
}

// immediate mode buttons call after button update
static void ButtonPressed()
{
    for (int i = 0; i < BUTTON_COUNT; ++i)
    {
        if (buttons[i].state & BUTTON_PRESSED)
        {
            switch (buttons[i].action)
            {
            default:
                printf("Button %d pressed\n", i);
            }
        }
    }
}

static void Draw()
{
    Sol_Button_Draw(buttons, 0, BUTTON_COUNT);
}