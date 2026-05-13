#pragma once
#include "sol/base.h"

typedef enum
{
    SOL_AUDIO_BEEP1,
    SOL_AUDIO_BEEP2,
    SOL_AUDIO_DIGILOAD,
    SOL_AUDIO_HIT,
    SOL_AUDIO_MENUMUSIC,
    SOL_AUDIO_SPACEGUN,
    SOL_AUDIO_WOONG,
    SOL_AUDIO_COUNT,
} SolAudioId;

int Sol_Audio_Init();
int Sol_Audio_LoadAll();

void Sol_Audio_Play(SolAudioId id);
void Sol_Audio_PlayAt(SolAudioId id, vec3s pos);