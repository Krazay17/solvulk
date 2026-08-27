#pragma once
#include "base.h"

#define MAX_AUDIOS 4

typedef struct World World;

typedef struct CompAudio
{
    int count, handle;
} CompAudio;

void Sol_World_Audio_Init(World *world);
void Sol_World_Audio_Add(World *world, int id, u32 audioId, float volume, u32 slot);
void Sol_World_Audio_Remove(World *world, int id, u32 slot);