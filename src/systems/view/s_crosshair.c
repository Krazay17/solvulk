#include "sol_core.h"

void Sol_View_Crosshair(World *world)
{
    WAdd2d(world) = Sol_Crosshair_Draw;
}

void Sol_Crosshair_Draw(World *world, double dt, double time)
{
    float width  = (float)Sol_GetState()->windowWidth / 2;
    float height = (float)Sol_GetState()->windowHeight / 2;
    Sol_Render_DrawRectangle(
        (vec4s){
            .x = width,
            .y = height,
            .z = 3,
            .w = 12,
        },
        (vec4s){255, 0, 0, 255}, 0);
    Sol_Render_DrawRectangle(
        (vec4s){
            .x = width,
            .y = height,
            .z = 12,
            .w = 3,
        },
        (vec4s){255, 0, 0, 255}, 0);
}

