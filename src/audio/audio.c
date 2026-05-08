/*
 * File: audio.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 * Audio!
*/
#include "sol_core.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#define SOL_CHANNELS 2
#define SOL_SAMPLERATE 48000

#define MAX_PLAYING_SOUNDS 32

static const char *audio_path[SOL_AUDIO_COUNT] = {
    [SOL_AUDIO_BEEP1] = "Beep1.wav",  [SOL_AUDIO_BEEP2] = "Beep2.wav",         [SOL_AUDIO_DIGILOAD] = "DigiLoad.mp3",
    [SOL_AUDIO_HIT] = "Hit.wav",      [SOL_AUDIO_MENUMUSIC] = "MenuMusic.mp3", [SOL_AUDIO_SPACEGUN] = "SpaceGun.mp3",
    [SOL_AUDIO_WOONG] = "Woong1.wav",
};

typedef struct
{
    void           *pcmData;
    ma_uint64       frameCount;
    size_t          pcmSize;
    ma_audio_buffer buffer; // decoded PCM, used as data source
    bool            loaded;
} SolAudio;

typedef struct
{
    ma_audio_buffer_ref bufferRef;
    ma_sound            sound;
    bool                inUse;
} PlayingSound;

static ma_engine    audio_engine;
static SolAudio     loaded_audio[SOL_AUDIO_COUNT];
static PlayingSound playing_pool[MAX_PLAYING_SOUNDS];

void      Sol_Load_Audios();
SolAudio *Parse_Audio(SolResource res, u32 id);

int Sol_Audio_Init(void)
{
    if (ma_engine_init(NULL, &audio_engine) != MA_SUCCESS)
        return -1;
    Sol_Load_Audios();
    return 0;
}

void Sol_Load_Audios(void)
{
    for (int i = 0; i < SOL_AUDIO_COUNT; i++)
    {
        SolResource res = Sol_LoadResource(audio_path[i]);
        if (!res.data)
            continue;

        Parse_Audio(res, i);

        if (res.isHeap)
            free(res.data);
    }
}

SolAudio *Parse_Audio(SolResource res, u32 id)
{
    SolAudio *audio = &loaded_audio[id];

    // Decode the entire audio file into PCM frames in memory
    ma_decoder        decoder;
    ma_decoder_config decConfig = ma_decoder_config_init(ma_format_f32, 2, 48000);
    if (ma_decoder_init_memory(res.data, res.size, &decConfig, &decoder) != MA_SUCCESS)
    {
        printf("Decode failed: %d\n", id);
        return NULL;
    }

    // Get the total PCM frame count
    ma_uint64 frameCount;
    ma_decoder_get_length_in_pcm_frames(&decoder, &frameCount);

    // Allocate PCM buffer and decode the whole thing
    size_t pcmSize    = frameCount * 2 * sizeof(float); // 2 channels, f32
    audio->pcmData    = malloc(pcmSize);
    audio->pcmSize    = pcmSize;
    audio->frameCount = frameCount;

    ma_decoder_read_pcm_frames(&decoder, audio->pcmData, frameCount, NULL);
    ma_decoder_uninit(&decoder);

    audio->loaded = true;
    printf("Loaded audio %d: %llu frames (%.2f sec)\n", id, frameCount, (float)frameCount / 48000.0f);
    return audio;
}

void Sol_PlayAudio(SolAudioId id)
{
    if (id >= SOL_AUDIO_COUNT || !loaded_audio[id].loaded)
        return;
    SolAudio *audio = &loaded_audio[id];

    // Find a free slot in the pool
    for (int i = 0; i < MAX_PLAYING_SOUNDS; i++)
    {
        PlayingSound *ps = &playing_pool[i];

        // Reuse if not in use, or if it's done playing
        if (ps->inUse)
        {
            if (ma_sound_at_end(&ps->sound))
            {
                ma_sound_uninit(&ps->sound);
                ma_audio_buffer_ref_uninit(&ps->bufferRef);

                ps->inUse = false;
            }
            else
            {
                continue;
            }
        }

        // Init this slot's own buffer ref pointing at the shared PCM data
        if (ma_audio_buffer_ref_init(ma_format_f32, 2, audio->pcmData, audio->frameCount, &ps->bufferRef) != MA_SUCCESS)
            return;

        // Init the sound from this slot's ref (independent cursor)
        if (ma_sound_init_from_data_source(&audio_engine, &ps->bufferRef, 0, NULL, &ps->sound) != MA_SUCCESS)
        {
            ma_audio_buffer_ref_uninit(&ps->bufferRef);
            return;
        }

        ma_sound_start(&ps->sound);
        ps->inUse = true;

        return;
    }
    // Pool full — drop the sound
}