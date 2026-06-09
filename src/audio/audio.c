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

#define MAX_PLAYING_SOUNDS 64

static const char *audio_path[SOL_AUDIO_COUNT] = {
    [SOL_AUDIO_BEEP1] = "Beep1.wav",         [SOL_AUDIO_BEEP2] = "Beep2.wav",
    [SOL_AUDIO_DIGILOAD] = "DigiLoad.mp3",   [SOL_AUDIO_HIT] = "Hit.wav",
    [SOL_AUDIO_MENUMUSIC] = "MenuMusic.mp3", [SOL_AUDIO_SPACEGUN] = "SpaceGun.mp3",
    [SOL_AUDIO_WOONG] = "Woong1.wav",        [SOL_AUDIO_FIREBALL] = "fireballUse.mp3",
    [SOL_AUDIO_DASH] = "dash.mp3",           [SOL_AUDIO_FIREBALLIMPACT] = "FireballImpact.mp3",
    [SOL_AUDIO_GOTHIT] = "PlayerHit.mp3",    [SOL_AUDIO_SWORDHIT] = "SwordHit.mp3",
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
    SolAudioId          id;
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
        .periodSizeInMilliseconds = 20,
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

static PlayingSound *Sol_Audio_Alloc(SolAudioId id, bool is3d, float volume, u32 maxConcurrent)
{
    if (id >= SOL_AUDIO_COUNT || !loaded_audio[id].loaded)
        return NULL;

    SolAudio *audio = &loaded_audio[id];

    if (maxConcurrent == 0)
        maxConcurrent = MAX_PLAYING_SOUNDS;

    // =========================================================================
    // STEP 1: CONCURRENCY GUARD & VOLUME DIMINISHING MATH
    // =========================================================================
    u32           activeCount    = 0;
    PlayingSound *oldestInstance = NULL;
    ma_uint64     oldestCursor   = 0;

    for (int i = 0; i < MAX_PLAYING_SOUNDS; i++)
    {
        PlayingSound *ps = &playing_pool[i];
        if (!ps->inUse)
            continue;

        if (ma_sound_at_end(&ps->sound))
        {
            ma_sound_uninit(&ps->sound);
            ma_audio_buffer_ref_uninit(&ps->bufferRef);
            ps->inUse = false;
            continue;
        }

        if (ps->id == id)
        {
            activeCount++;

            ma_uint64 currentCursor;
            ma_sound_get_cursor_in_pcm_frames(&ps->sound, &currentCursor);
            if (currentCursor >= oldestCursor)
            {
                oldestCursor   = currentCursor;
                oldestInstance = ps;
            }
        }
    }

    if (activeCount >= maxConcurrent && oldestInstance != NULL)
    {
        ma_sound_uninit(&oldestInstance->sound);
        ma_audio_buffer_ref_uninit(&oldestInstance->bufferRef);
        oldestInstance->inUse = false;
        // Balance counter after recycling an instance
        activeCount--;
    }

    // =========================================================================
    // STEP 2: STANDARD RESOURCE ACQUISITION
    // =========================================================================
    for (int i = 0; i < MAX_PLAYING_SOUNDS; i++)
    {
        PlayingSound *ps = &playing_pool[i];
        if (ps->inUse)
            continue;

        if (ma_audio_buffer_ref_init(DEVICE_FORMAT, DEVICE_CHANNELS, audio->pcmData, audio->frameCount,
                                     &ps->bufferRef) != MA_SUCCESS)
            return NULL;

        ma_uint32 flags = MA_SOUND_FLAG_DECODE;
        if (!is3d)
        {
            flags |= MA_SOUND_FLAG_NO_SPATIALIZATION;
        }

        if (ma_sound_init_from_data_source(&audio_engine, &ps->bufferRef, flags, NULL, &ps->sound) != MA_SUCCESS)
        {
            ma_audio_buffer_ref_uninit(&ps->bufferRef);
            return NULL;
        }

        float volumeMultiplier = 1.0f / (1.0f + (float)(activeCount * activeCount));
        ma_sound_set_volume(&ps->sound, volume * volumeMultiplier);

        ps->id    = id;
        ps->inUse = true;
        // --- THE FIX: Equalized Redistribution Pass ---
        // Increment count to include the new instance we just built
        u32 totalInstances = activeCount + 1;

        // Calculate the even share of the requested volume budget
        float evenedVolume = volume / (float)totalInstances;

        // Apply this evened volume to EVERY active instance of this sound right now
        for (int j = 0; j < MAX_PLAYING_SOUNDS; j++)
        {
            if (playing_pool[j].inUse && playing_pool[j].id == id)
            {
                ma_sound_set_volume(&playing_pool[j].sound, evenedVolume);
            }
        }

        return ps;
    }

    return NULL;
}

void Sol_Audio_Play(SolAudioId id, float volume, float seek, u32 concurrent)
{
    ma_uint64     seekFrame = SOL_SECONDS_TO_FRAMES(seek);
    PlayingSound *ps        = Sol_Audio_Alloc(id, false, volume, concurrent);
    if (!ps)
        return;

    if (seekFrame > 0)
    {
        ma_sound_seek_to_pcm_frame(&ps->sound, seekFrame);
    }

    ma_sound_start(&ps->sound);
}

void Sol_Audio_PlayAt(SolAudioId id, vec3s pos, float volume, float seek, u32 concurrent)
{
    ma_uint64     seekFrame = SOL_SECONDS_TO_FRAMES(seek);
    PlayingSound *ps        = Sol_Audio_Alloc(id, true, volume, concurrent);
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
