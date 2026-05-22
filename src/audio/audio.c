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

#define DEVICE_FORMAT ma_format_f32
#define DEVICE_CHANNELS 2
#define DEVICE_SAMPLE_RATE 48000
#define DEVICE_SAMPLE_RATE_F 48000.0f
#define SOL_SECONDS_TO_FRAMES(sec) ((ma_uint64)((float)(sec) * 48000.0f))

#define MAX_SINEWAVES 4

#define MAX_PLAYING_SOUNDS 32

static const char *audio_path[SOL_AUDIO_COUNT] = {
    [SOL_AUDIO_BEEP1] = "Beep1.wav",         [SOL_AUDIO_BEEP2] = "Beep2.wav",
    [SOL_AUDIO_DIGILOAD] = "DigiLoad.mp3",   [SOL_AUDIO_HIT] = "Hit.wav",
    [SOL_AUDIO_MENUMUSIC] = "MenuMusic.mp3", [SOL_AUDIO_SPACEGUN] = "SpaceGun.mp3",
    [SOL_AUDIO_WOONG] = "Woong1.wav",        [SOL_AUDIO_FIREBALL] = "fireballUse.mp3",
    [SOL_AUDIO_DASH] = "dash.mp3",           [SOL_AUDIO_FIREBALLIMPACT] = "FireballImpact.mp3",

};

typedef struct
{
    void           *pcmData;
    ma_uint64       frameCount;
    size_t          pcmSize;
    ma_audio_buffer buffer; // decoded PCM, used as data source
    bool            loaded;
    float           duration;
} SolAudio;

typedef struct
{
    ma_audio_buffer_ref bufferRef;
    ma_sound            sound;
    bool                inUse;
} PlayingSound;

typedef struct
{
    ma_waveform wave;
    ma_sound    sound;
    bool        active;
} SoundWave;

static SolAudio loaded_audio[SOL_AUDIO_COUNT];

static ma_engine    audio_engine;
static PlayingSound playing_pool[MAX_PLAYING_SOUNDS];

static SoundWave sine_pool[MAX_SINEWAVES];

static SolAudio *Parse_Audio(SolResource res, u32 id);
static void      data_callback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount)
{
    ma_waveform *pSineWave = (ma_waveform *)pDevice->pUserData;
    if (pSineWave)
        ma_waveform_read_pcm_frames(pSineWave, pOutput, frameCount, NULL);
}

int Sol_Audio_Init(void)
{
    ma_engine_config eConfig = {
        .periodSizeInMilliseconds = 15,
    };
    if (ma_engine_init(&eConfig, &audio_engine) != MA_SUCCESS)
        return -1;
    ma_engine_set_volume(&audio_engine, 0.5);
    ma_engine_listener_set_world_up(&audio_engine, 0, 0.0f, 1.0f, 0.0f);

    Sol_Audio_LoadAll();

    for (int i = 0; i < MAX_SINEWAVES; i++)
    {
        ma_waveform_config waveConfig = ma_waveform_config_init(ma_format_f32, DEVICE_CHANNELS, DEVICE_SAMPLE_RATE,
                                                                ma_waveform_type_sine, 0.1, 444);
        ma_waveform_init(&waveConfig, &sine_pool[i].wave);

        ma_result result =
            ma_sound_init_from_data_source(&audio_engine, &sine_pool[i].wave, 0, NULL, &sine_pool[i].sound);
    }

    return 0;
}

void Sol_Audio_Update(vec3s listenerPos, vec3s listenerDir)
{
    ma_engine_listener_set_position(&audio_engine, 0, listenerPos.x, listenerPos.y, listenerPos.z);
    ma_engine_listener_set_direction(&audio_engine, 0, listenerDir.x, listenerDir.y, listenerDir.z);
}

int Sol_Audio_LoadAll(void)
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

    return 0;
}

static SolAudio *Parse_Audio(SolResource res, u32 id)
{
    SolAudio *audio = &loaded_audio[id];

    // Decode the entire audio file into PCM frames in memory
    ma_decoder        decoder;
    ma_decoder_config decConfig = ma_decoder_config_init(DEVICE_FORMAT, DEVICE_CHANNELS, DEVICE_SAMPLE_RATE);
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

    audio->loaded   = true;
    audio->duration = (float)frameCount / DEVICE_SAMPLE_RATE_F;
    printf("Loaded audio %d: %llu frames (%.2f sec)\n", id, frameCount, audio->duration);
    return audio;
}

// Internal helper: Finds a slot, initializes the buffer, and handles 3D setup
static PlayingSound *Sol_Audio_Alloc(SolAudioId id, bool is3D)
{
    if (id >= SOL_AUDIO_COUNT || !loaded_audio[id].loaded)
        return NULL;

    SolAudio *audio = &loaded_audio[id];

    for (int i = 0; i < MAX_PLAYING_SOUNDS; i++)
    {
        PlayingSound *ps = &playing_pool[i];

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

        if (ma_audio_buffer_ref_init(DEVICE_FORMAT, DEVICE_CHANNELS, audio->pcmData, audio->frameCount,
                                     &ps->bufferRef) != MA_SUCCESS)
            return NULL;

        if (ma_sound_init_from_data_source(&audio_engine, &ps->bufferRef, 0, NULL, &ps->sound) != MA_SUCCESS)
        {
            ma_audio_buffer_ref_uninit(&ps->bufferRef);
            return NULL;
        }

        ps->inUse = true;
        return ps; // Return the entire tracking wrapper
    }

    return NULL; // Pool full
}

// seekFrame: The exact PCM frame to start from (Pass 0 to play from the beginning)
void Sol_Audio_PlayAt(SolAudioId id, vec3s pos, float seek)
{
    ma_uint64     seekFrame = SOL_SECONDS_TO_FRAMES(seek);
    bool          is3d      = pos.x > 0;
    PlayingSound *ps        = Sol_Audio_Alloc(id, is3d);
    if (!ps)
        return;

    // 1. Set position first to avoid thread popping
    ma_sound_set_position(&ps->sound, pos.x, pos.y, pos.z);

    // 2. Seek if a specific start time was requested
    if (seekFrame > 0)
    {
        ma_sound_seek_to_pcm_frame(&ps->sound, seekFrame);
    }

    // 3. Start mixing
    ma_sound_start(&ps->sound);
}

void Sol_Audio_SetVolume(float volume)
{
    ma_engine_set_volume(&audio_engine, volume);
}

void Sol_Audio_PlaySine(int i)
{
    SoundWave *sound = &sine_pool[i];
    sound->active    = !sound->active;
    if (sound->active)
        ma_sound_start(&sound->sound);
    else
        ma_sound_stop(&sound->sound);
}

void Sol_Audio_SetVolumeSine(int i, float volume)
{
    ma_sound_set_volume(&sine_pool[i].sound, volume);
    if (!sine_pool[i].active)
    {
        sine_pool->active = true;
        ma_sound_start(&sine_pool[i].sound);
    }
}

void Sol_Audio_SineFreq(int i, float freq)
{
    ma_waveform_set_frequency(&sine_pool[i].wave, (double)freq * (float)(i + 1));
}
