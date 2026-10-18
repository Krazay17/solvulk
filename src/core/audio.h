#pragma once
#include "types.h"
#include "platform/platform.h"

typedef struct SolAudio SolAudio;
typedef struct ma_sound ma_sound;

typedef struct
{
    u32 index;
    u32 generation;
} ScAudioHandle;

int       Sol_Audio_Init();
SolAudio *Parse_Audio(SolResource res, u32 id);

ScAudioHandle Sol_Audio_Play(SoundKind id, float volume, float seek, u32 concurrent);
ScAudioHandle Sol_Audio_PlayAt(SoundKind id, vec3s pos, float volume, float seekFrame, u32 concurrent);
void          Sol_Audio_SetVolume(float volume);
void          Sol_Update_Audio_FromView();

void Sol_Audio_UpdateListener(vec3s pos, vec3s dir);
void Sol_Audio_SetSlotPosition(ScAudioHandle handle, vec3s pos);
void Sol_Audio_SetSlotVolume(ScAudioHandle handle, float volume);
void Sol_Audio_SetSlotPitch(ScAudioHandle handle, float pitch);
void Sol_Audio_SetSlotLooping(ScAudioHandle handle, bool loop);
void Sol_Audio_StopSlot(ScAudioHandle handle);