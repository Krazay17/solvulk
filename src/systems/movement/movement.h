#pragma once

typedef struct
{
    MoveConfigId configId;
} MovementDesc;
void Sol_Movement_Init(World *world);
void Sol_Movement_Add(World *world, int id, MovementDesc desc);
void Sol_System_Movement_2d_Step(World *world, double dt, double time);
void Sol_System_Movement_3d_Step(World *world, double dt, double time);
void Sol_Movement_SetSpeedMod(World *world, int id, float amnt);