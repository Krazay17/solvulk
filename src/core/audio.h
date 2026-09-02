#pragma once
#include "types.h"

typedef struct ma_sound ma_sound;

typedef struct
{
    u32 index;
    u32 generation;
} ScAudioHandle;

int Sol_Audio_Init();
int Sol_Audio_LoadAll();

ScAudioHandle Sol_Audio_Play(ScAudioId id, float volume, float seek, u32 concurrent);
ScAudioHandle Sol_Audio_PlayAt(ScAudioId id, vec3s pos, float volume, float seekFrame, u32 concurrent);
void           Sol_Audio_SetVolume(float volume);
void Audio_Update_Listener(vec3s listenerPos, vec3s listenerDir);
void Sol_Update_Audio_FromView();

void Sol_Audio_SetSlotPosition(ScAudioHandle handle, vec3s pos);
void Sol_Audio_SetSlotVolume(ScAudioHandle handle, float volume);
void Sol_Audio_SetSlotPitch(ScAudioHandle handle, float pitch);
void Sol_Audio_SetSlotLooping(ScAudioHandle handle, bool loop);
void Sol_Audio_StopSlot(ScAudioHandle handle);
