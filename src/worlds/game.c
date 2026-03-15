#include "world.h"
#include "loader.h"
#include "render.h"

static float rotation = 0.0f;

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
    Sol_Init_Triangle();
}

static void Tick(double dt, double time)
{
    rotation += 5.0f * dt;
}

static void Draw()
{
    SolCamera cam = {
        .position = {0.0f, 0.0f, 2.0f},
        .target = {0.0f, 0.0f, 0.0f},
        .fov = 60.0f,
        .nearClip = 0.1f,
        .farClip = 100.0f,
    };

    Sol_Begin_Draw();

    Sol_DrawTriangle(&cam, rotation);

    Sol_End_Draw();
}