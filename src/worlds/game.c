#include "world.h"
#include "loader.h"
#include "render.h"
#include "button.h"

#define BUTTON_COUNT 6

static float rotation = 0.0f;
static SolButton buttons = {0};

static void Init();
static void Tick(double dt, double time);
static void Draw();
static GameState gameState = {
    .entities = {0},
    .entCount = 0,
};

static World game = {
    .init = Init,
    .tick = Tick,
    .draw = Draw,
    .state = &gameState,
};

World *GetGame()
{
    return &game;
}

static void Init()
{
    for (int i = 0; i < BUTTON_COUNT; ++i)
    {
        int xOffset = 50;
        buttons.rect[i] = (SolRect){0, i * 80 + xOffset, 100, 50};
        buttons.color[i] = (SolColor){255, 0, 0, 55};
        buttons.textColor[i] = (SolColor){255, 255, 0, 255};
        buttons.drawOrder[i] = i;

        switch (i)
        {
        case 0:
            buttons.text[i] = "QUIT";
            buttons.action[i] = BUTTON_ACTION_QUIT;
            break;
        case 1:
            buttons.text[i] = "Colin";
            buttons.action[i] = BUTTON_ACTION_QUIT;
            buttons.color[i] = (SolColor){111, 255, 0, 255};
            buttons.textColor[i] = (SolColor){0, 123, 255, 255};
            break;
        case 2:
            buttons.text[i] = "Is";
            buttons.action[i] = BUTTON_ACTION_QUIT;
            break;
        case 3:
            buttons.text[i] = "An";
            buttons.action[i] = BUTTON_ACTION_QUIT;
            break;
        case 4:
            buttons.text[i] = "Idiot";
            buttons.action[i] = BUTTON_ACTION_QUIT;
            buttons.textColor[i] = (SolColor){0, 5, 255, 255};
            break;
        default:
            buttons.text[i] = "BUTTON";
            buttons.action[i] = BUTTON_ACTION_NONE;
        }
    }
}

static void Tick(double dt, double time)
{
    rotation += 5.0f * dt;

    Sol_Camera_Update((vec3){0, 0, 5}, (vec3){0, 0, 0});

    Sol_Button_Update(&buttons, 0, BUTTON_COUNT, dt);
}

static void Draw()
{
    Sol_DrawModel(&GetBank()->models.gpuWizard, (vec3){0, -1, 0}, rotation);

    Sol_Button_Draw(&buttons);

    // Sol_Draw_Text("TESTING!!", 0, 0, 32.0f, (SolColor){0, 255, 0, 255});
}