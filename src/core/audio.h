#pragma once
#include "types.h"

typedef struct ma_sound ma_sound;

typedef struct
{
    u32 index;
    u32 generation;
} SolAudioHandle;

int Sol_Audio_Init();
int Sol_Audio_LoadAll();

SolAudioHandle Sol_Audio_Play(SolAudioId id, float volume, float seek, u32 concurrent);
SolAudioHandle Sol_Audio_PlayAt(SolAudioId id, vec3s pos, float volume, float seekFrame, u32 concurrent);
void           Sol_Audio_SetVolume(float volume);
void           Sol_Audio_Update(vec3s listenerPos, vec3s listenerDir);

void Sol_Audio_SetSlotPosition(SolAudioHandle handle, vec3s pos);
void Sol_Audio_SetSlotVolume(SolAudioHandle handle, float volume);
void Sol_Audio_SetSlotPitch(SolAudioHandle handle, float pitch);
void Sol_Audio_SetSlotLooping(SolAudioHandle handle, bool loop);
void Sol_Audio_StopSlot(SolAudioHandle handle);
