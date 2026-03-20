#include <stdio.h>

#include "world.h"
#include "systems.h"
#include "loader.h"
#include "render.h"
#include "solglobals.h"

#define BUTTON_COUNT 20
#define MODEL_COUNT 5000

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

static void ButtonPressed();

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
        float startX = 100.0f;
        float startY = 100.0f;
        float xOffset = col * 110 + startX;
        float yOffset = row * 60 + startY;
        buttons.rect[i] = (SolRect){xOffset, yOffset, 100, 50};
        buttons.color[i] = (SolColor){255, 0, 0, 55};
        buttons.textColor[i] = (SolColor){255, 255, 0, 255};
        buttons.drawOrder[i] = i;

        switch (i)
        {
        case 0:
            buttons.text[i] = "QUIT";
            buttons.action[i] = BUTTON_ACTION_QUIT;
            buttons.callback[i] = Sol_Quit;
            break;
        case 1:
            buttons.text[i] = "Colin";
            buttons.color[i] = (SolColor){111, 255, 0, 255};
            buttons.textColor[i] = (SolColor){0, 123, 255, 255};
            break;
        case 2:
            buttons.text[i] = "Is";
            break;
        case 3:
            buttons.text[i] = "An";
            break;
        case 4:
            buttons.text[i] = "Idiot";
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

//immediate mode buttons call after button update
static void ButtonPressed()
{
    for (int i = 0; i < MAX_BUTTONS; ++i)
    {
        if (buttons.state[i] & BUTTON_PRESSED)
        {
            switch (i)
            {
            case 0:
                printf("Button 0 pressed\n");
                break;
            case 1:
                printf("Button 1 pressed\n");
                break;
            }
        }
    }
}

static void Draw()
{
    for (int i = 0; i < MODEL_COUNT; ++i)
        Sol_DrawModel(GetBank()->models.wizard, (vec3){sin(i), -1, -i}, rotation);

    Sol_Button_Draw(&buttons);
}