/*
 * File: system.h
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-09-11
 *
 */
#pragma once
#include "sol/types.h"

// Systems
void Player_Init(World *world);
void Move3_Init(World *world);
void Anim_Init(World *world);
void Camera_Init(World *world);
void Model_Init(World *world);

void Player_Tick(World *world);
void Interact_Update(World *world);
void Parent_Update(World *world);
void Abilitybar_Update(World *world);

void Move3_Step(World *world);
void Move2_Step(World *world);

void Body3_Update(World *world);
void Body2_Step(World *world);

void Ability_Step(World *world);
void Projectile_Step(World *world);
void Zone_Update(World *world);
void Combat_Step(World *world);
void Ai_Step(World *world);
void Interact_Step(World *world);

void Fx_Update(World *world);
void Hook_Tick(World *world);
void Anim_Tick(World *world);
void Facing_Tick(World *world);
void Camera_Tick(World *world);
void Emitter_Update(World *world);
void Timer_Update(World *world);

void Particle_Draw(World *world);
void Scoreboard_Draw(World *world);
void Model_Render(World *world);
void Ability_Draw(World *world);
void View3_Draw(World *world);
void View2_Draw(World *world);
void Debug_Tick(World *world);
void Debug_Draw3(World *world);
void Debug_Draw2(World *world);
