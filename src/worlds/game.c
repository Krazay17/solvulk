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
}

static void Tick(double dt, double time)
{
    rotation += 5.0f * dt;

    Sol_Camera_Update((vec3){0, 0, 5}, (vec3){0, 0, 0});
}

static void Draw()
{
    Sol_Begin_Draw();

    Sol_DrawModel(&GetBank()->models.gpuWizard, (vec3){0, -1, 0}, rotation);

    Sol_Draw_Rectangle((SolRect){0, 0, 100, 50}, (SolColor){255, 0, 0, 255});

    Sol_End_Draw();
}