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
void Combat_Init(World *world);
void Player_Init(World *world);
void Player_Deinit(World *world);
void Move3_Init(World *world);
void Move3_Deinit(World *world);
void Body3_Init(World *world);
void Body3_Deinit(World *world);
void Anim_Init(World *world);
void Anim_Deinit(World *world);
void Camera_Init(World *world);
void Camera_Deinit(World *world);
void Model_Init(World *world);
void Model_Deinit(World *world);
void Debug_Init(World *world);
void Debug_Deinit(World *world);

void Player_Tick(World *world, double dt);
void Interact_Update(World *world, double dt);
void Slider_Update(World *world, double dt);
void Parent_Update(World *world, double dt);

void Move3_Step(World *world, double dt);
void Move2_Step(World *world, double dt);
void Body3_Step(World *world, double dt);
void Body2_Step(World *world, double dt);
void Ability_Step(World *world, double dt);
void Combat_Step(World *world, double dt);
void Ai_Step(World *world, double dt);
void Interact_Body_Step(World *world, double dt);

void Hook_Tick(World *world, double dt);
void Anim_Tick(World *world, double dt);
void Facing_Tick(World *world, double dt);
void Camera_Tick(World *world, double dt);

void Scoreboard_Draw(World *world, double dt);
void Model_Render(World *world, double dt);
void Ability_Draw(World *world, double dt);
void View3_Draw(World *world, double dt);
void View2_Draw(World *world, double dt);
void View2_Healthbar(World *world, double dt);
void View2_Abilitybar(World *world, double dt);
void Debug_Tick(World *world, double dt);
void Debug_Draw3(World *world, double dt);
void Debug_Draw2(World *world, double dt);
