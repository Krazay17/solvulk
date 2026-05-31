#pragma once
#include "sol/types.h"

typedef struct
{
    const char *text;
    vec3s       pos;
    float       size;
    vec4s       color;
    SolFontKind font;
    bool        billboard; // face camera or use rotation
    versors     rotation;  // if not billboard
} Text3DDesc;

void Sol_View_Init(World *world);

void Sol_View_Crosshair(World *world);

void Sol_View_Buff(World *world);
void Sol_View_Ability(World *world);
void Sol_View_Healthbar(World *world);
void Sol_View_Fx(World *world);

void Fx_Event(World *world, double dt, double time);

void Sol_View_Buff_Draw(World *world, double dt, double time);
void Sol_View_Ability_Draw(World *world, double dt, double time);
void Sol_View_Healthbar_Draw(World *world, double dt, double time);
void Sol_View_Crosshair_Draw(World *world, double dt, double time);
void Sol_View_Particle_Draw(World *world, double dt, double time);

void Sol_Render_DrawText3D(Text3DDesc desc);