#pragma once
#include "sol/base.h"

typedef struct ma_sound ma_sound;

typedef enum
{
    SOL_AUDIO_BEEP1,
    SOL_AUDIO_BEEP2,
    SOL_AUDIO_DIGILOAD,
    SOL_AUDIO_HIT,
    SOL_AUDIO_GOTHIT,
    SOL_AUDIO_SWORD_SWING,
    SOL_AUDIO_PARRY,
    SOL_AUDIO_WOODCOCK,
    SOL_AUDIO_LIGHTNINGHIT,
    SOL_AUDIO_SWORDHIT,
    SOL_AUDIO_MENUMUSIC,
    SOL_AUDIO_SPACEGUN,
    SOL_AUDIO_WOONG,
    SOL_AUDIO_FIREBALL,
    SOL_AUDIO_DASH,
    SOL_AUDIO_FIREBALLIMPACT,
    SOL_AUDIO_COUNT,
} SolAudioId;

int Sol_Audio_Init();
int Sol_Audio_LoadAll();

void Sol_Audio_Play(SolAudioId id, float volume, float seek, u32 concurrent);
void Sol_Audio_PlayAt(SolAudioId id, vec3s pos, float volume, float seekFrame, u32 concurrent);
void Sol_Audio_SetVolume(float volume);
void Sol_Audio_PlaySine(int i);
void Sol_Audio_SineFreq(int i, float freq);
void Sol_Audio_SetVolumeSine(int i, float volume);
void Sol_Audio_Update(vec3s listenerPos, vec3s listenerDir);
