#include "world.h"
#include "systems.h"
#include "loader.h"
#include "render.h"
#include "button.h"

#define BUTTON_COUNT 140
#define MODEL_COUNT 1000

static vec3 playerPos = {0, 4, 10};
static float rotation = 0.0f;
static float camPos = 10.0f;
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
        int cols = 12;
        int col = i % cols;
        int row = i / cols;
        float xOffset = col * 110; // 100 wide + 10 gap
        float yOffset = row * 60;  // 50 tall + 10 gap
        float startX = 100.0f;
        float startY = 100.0f;
        buttons.rect[i] = (SolRect){xOffset + startX, yOffset + startY, 100, 50};
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

    PlayerState *playerState = Sol_GetPlayerState();
    if (playerState->actionState & ACTION_FWD)
    {
        playerPos[2] -= 10.0f * dt;
    }
    else if (playerState->actionState & ACTION_BWD)
    {
        playerPos[2] += 10.0f * dt;
    }

    Sol_Camera_Update((vec3){0, 7, playerPos[2]}, (vec3){0, 0, 0});

    Sol_Button_Update(&buttons, 0, BUTTON_COUNT, dt);
}

static void Draw()
{
    for (int i = 0; i < MODEL_COUNT; ++i)
    {
        Sol_DrawModel(&GetBank()->models.gpuWizard, (vec3){sin(i), -1, -i}, rotation);
    }

    Sol_Button_Draw(&buttons);

    // Sol_Draw_Text("TESTING!!", 0, 0, 32.0f, (SolColor){0, 255, 0, 255});
}