#include "world.h"
#include "loader.h"
#include "render.h"
#include "button.h"

static float rotation = 0.0f;
static SolButton buttons = {
    .rect[0] = (SolRect){0, 0, 100, 50},
    .color[0] = (SolColor){255, 0, 0, 255},
};

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
}

static void Tick(double dt, double time)
{
    rotation += 5.0f * dt;

    Sol_Camera_Update((vec3){0, 0, 5}, (vec3){0, 0, 0});

    Sol_Button_Update(&buttons, 0, 1, dt);
}

static void Draw()
{
    Sol_Begin_Draw();

    Sol_DrawModel(&GetBank()->models.gpuWizard, (vec3){0, -1, 0}, rotation);

    Sol_Button_Draw(&buttons);

    Sol_Draw_Text("TESTING!!", 0,0,32.0f, (SolColor){0,255,0,255});

    Sol_End_Draw();
}