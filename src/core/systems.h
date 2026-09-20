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
void Camera_Init(World *world);
void Model_Init(World *world);

void Player_Tick(World *world, double dt);
void Interact_Update(World *world, double dt);
void Parent_Update(World *world, double dt);
void Abilitybar_Update(World *world, double dt);

void Move3_Step(World *world, double dt);
void Move2_Step(World *world, double dt);

void Body3_Update(World *world, double dt);
void Body2_Step(World *world, double dt);

void Buff_Update(World *world, double dt);
void Ability_Step(World *world, double dt);
void Projectile_Step(World *world, double dt);
void Zone_Update(World *world, double dt);
void Combat_Step(World *world, double dt);
void Ai_Step(World *world, double dt);
void Interact_Step(World *world, double dt);

void Fx_Update(World *world, double dt);
void Hook_Tick(World *world, double dt);
void Anim_Tick(World *world, double dt);
void Facing_Tick(World *world, double dt);
void Camera_Tick(World *world, double dt);
void Emitter_Update(World *world, double dt);
void Timer_Update(World *world, double dt);

void Particle_Draw(World *world);
void Scoreboard_Draw(World *world);
void Model_Render(World *world);
void Ability_Draw(World *world);
void View3_Draw(World *world);
void View2_Draw(World *world);
void Debug_Tick(World *world);
void Debug_Draw3(World *world);
void Debug_Draw2(World *world);
