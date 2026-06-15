/*
 * File: audio.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-05-08
 */
#include "sol_core.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#define DEVICE_FORMAT ma_format_f32
#define DEVICE_CHANNELS 2
#define DEVICE_SAMPLE_RATE 48000
#define DEVICE_SAMPLE_RATE_F 48000.0f
#define SOL_SECONDS_TO_FRAMES(sec) ((ma_uint64)((float)(sec) * 48000.0f))

#define MAX_PLAYING_SOUNDS 512

static const char *audio_path[SOL_AUDIO_COUNT] = {
    [SOL_AUDIO_BEEP1]          = "Beep1.wav",
    [SOL_AUDIO_BEEP2]          = "Beep2.wav",
    [SOL_AUDIO_DIGILOAD]       = "DigiLoad.mp3",
    [SOL_AUDIO_HIT]            = "Hit.wav",
    [SOL_AUDIO_MENUMUSIC]      = "MenuMusic.mp3",
    [SOL_AUDIO_SPACEGUN]       = "SpaceGun.mp3",
    [SOL_AUDIO_WOONG]          = "Woong1.wav",
    [SOL_AUDIO_FIREBALL]       = "fireballUse.mp3",
    [SOL_AUDIO_DASH]           = "dash.mp3",
    [SOL_AUDIO_FIREBALLIMPACT] = "FireballImpact.mp3",
    [SOL_AUDIO_GOTHIT]         = "PlayerHit.mp3",
    [SOL_AUDIO_SWORDHIT]       = "SwordHit.mp3",
    [SOL_AUDIO_SWORD_SWING]    = "HeavySword.mp3",
    [SOL_AUDIO_PARRY]          = "Parry.mp3",
    [SOL_AUDIO_WOODCOCK]       = "WoodCock.mp3",
    [SOL_AUDIO_LIGHTNINGHIT]   = "LightningHit.mp3",
    [SOL_AUDIO_LASER]          = "Laser.mp3",
};

const SolAudioHandle INVALID_AUDIO_HANDLE = {.index = 0, .generation = 0};

typedef struct
{
    void     *pcmData;
    ma_uint64 frameCount;
    bool      loaded;
    float     duration;
} SolAudio;

typedef struct
{
    ma_audio_buffer_ref bufferRef; // per-instance view into shared pcmData
    ma_sound            sound;
    SolAudioId          id;
    u32                 generation;
    bool                inUse;
} PlayingSound;

static SolAudio     loaded_audio[SOL_AUDIO_COUNT];
static ma_engine    audio_engine;
static PlayingSound playing_pool[MAX_PLAYING_SOUNDS];

// --- Handle validation ---

static bool Sol_Audio_IsHandleValid(SolAudioHandle handle)
{
    if (handle.generation == 0)
        return false;
    if (handle.index >= MAX_PLAYING_SOUNDS)
        return false;
    PlayingSound *ps = &playing_pool[handle.index];
    if (!ps->inUse)
        return false;
    if (ps->generation != handle.generation)
        return false;
    // Sound finished naturally — treat as invalid but don't force-free here
    return true;
}

// --- Slot cleanup (internal) ---

static void Slot_Uninit(PlayingSound *ps)
{
    if (!ps->inUse)
        return;
    ma_sound_stop(&ps->sound);
    ma_sound_uninit(&ps->sound);
    ma_audio_buffer_ref_uninit(&ps->bufferRef);
    ps->inUse = false;
    ps->id    = 0;
}

// --- Core allocator ---

static SolAudioHandle Sol_Audio_Alloc(SolAudioId id, bool is3d, float volume, u32 maxConcurrent)
{
    if (id >= SOL_AUDIO_COUNT || !loaded_audio[id].loaded)
        return INVALID_AUDIO_HANDLE;

    SolAudio *audio = &loaded_audio[id];
    if (maxConcurrent == 0)
        maxConcurrent = MAX_PLAYING_SOUNDS;

    // --- Pass 1: reap finished sounds, count active, track oldest ---
    u32           activeCount    = 0;
    PlayingSound *evictCandidate = NULL;
    ma_uint64     oldestCursor   = UINT64_MAX;

    for (int i = 0; i < MAX_PLAYING_SOUNDS; i++)
    {
        PlayingSound *ps = &playing_pool[i];
        if (!ps->inUse)
            continue;

        if (ma_sound_at_end(&ps->sound))
        {
            Slot_Uninit(ps);
            continue;
        }

        if (ps->id == id)
        {
            activeCount++;
            ma_uint64 cursor = 0;
            ma_sound_get_cursor_in_pcm_frames(&ps->sound, &cursor);
            // Track the furthest-along (oldest) instance for eviction
            if (cursor <= oldestCursor)
            {
                oldestCursor   = cursor;
                evictCandidate = ps;
            }
        }
    }

    // --- Pass 2: evict oldest if over concurrent limit ---
    if (activeCount >= maxConcurrent && evictCandidate)
    {
        Slot_Uninit(evictCandidate);
        activeCount--;
    }

    // --- Pass 3: claim a free slot ---
    for (int i = 0; i < MAX_PLAYING_SOUNDS; i++)
    {
        PlayingSound *ps = &playing_pool[i];
        if (ps->inUse)
            continue;

        // Each instance gets its own buffer ref — independent seek cursors,
        // no shared-state race between concurrent plays of the same sound
        if (ma_audio_buffer_ref_init(DEVICE_FORMAT, DEVICE_CHANNELS, audio->pcmData, audio->frameCount,
                                     &ps->bufferRef) != MA_SUCCESS)
            return INVALID_AUDIO_HANDLE;

        ma_uint32 flags = 0; // no MA_SOUND_FLAG_DECODE — already decoded PCM
        if (!is3d)
            flags |= MA_SOUND_FLAG_NO_SPATIALIZATION;

        if (ma_sound_init_from_data_source(&audio_engine, &ps->bufferRef, flags, NULL, &ps->sound) != MA_SUCCESS)
        {
            ma_audio_buffer_ref_uninit(&ps->bufferRef);
            return INVALID_AUDIO_HANDLE;
        }

        // After claiming the slot and before ma_sound_start:
        
        ps->id    = id;
        ps->inUse = true;
        ps->generation++;
        if (ps->generation == 0)
        ps->generation = 1; // never emit generation 0
        
        u32   totalInstances   = activeCount + 1; // includes the one just allocated
        float equalPowerVolume = volume / sqrtf((float)totalInstances);

        // Retroactively level all active instances of this sound
        for (int j = 0; j < MAX_PLAYING_SOUNDS; j++)
        {
            PlayingSound *other = &playing_pool[j];
            if (other->inUse && other->id == id)
                ma_sound_set_volume(&other->sound, equalPowerVolume);
        }

        return (SolAudioHandle){.index = i, .generation = ps->generation};
    }

    return INVALID_AUDIO_HANDLE;
}

// --- Init ---

int Sol_Audio_Init(void)
{
    ma_engine_config cfg = {.periodSizeInMilliseconds = 20};
    if (ma_engine_init(&cfg, &audio_engine) != MA_SUCCESS)
        return -1;

    ma_engine_set_volume(&audio_engine, 0.1f);
    ma_engine_listener_set_world_up(&audio_engine, 0, 0.0f, 1.0f, 0.0f);

    for (int i = 0; i < MAX_PLAYING_SOUNDS; i++)
    {
        playing_pool[i].inUse      = false;
        playing_pool[i].generation = 1; // start at 1 so 0 is always invalid
        playing_pool[i].id         = 0;
    }

    Sol_Audio_LoadAll();
    return 0;
}

void Sol_Audio_Update(vec3s listenerPos, vec3s listenerDir)
{
    ma_engine_listener_set_position(&audio_engine, 0, listenerPos.x, listenerPos.y, listenerPos.z);
    ma_engine_listener_set_direction(&audio_engine, 0, listenerDir.x, listenerDir.y, listenerDir.z);
}

// --- Loading ---

static SolAudio *Parse_Audio(SolResource res, u32 id)
{
    SolAudio *audio = &loaded_audio[id];

    ma_decoder        decoder;
    ma_decoder_config decCfg = ma_decoder_config_init(DEVICE_FORMAT, DEVICE_CHANNELS, DEVICE_SAMPLE_RATE);
    if (ma_decoder_init_memory(res.data, res.size, &decCfg, &decoder) != MA_SUCCESS)
    {
        printf("Decode failed: %d\n", id);
        return NULL;
    }

    ma_uint64 frameCount = 0;
    ma_decoder_get_length_in_pcm_frames(&decoder, &frameCount);

    size_t pcmSize = frameCount * DEVICE_CHANNELS * sizeof(float);
    void  *pcmData = malloc(pcmSize);
    ma_decoder_read_pcm_frames(&decoder, pcmData, frameCount, NULL);
    ma_decoder_uninit(&decoder);

    audio->pcmData    = pcmData;
    audio->frameCount = frameCount;
    audio->loaded     = true;
    audio->duration   = (float)frameCount / DEVICE_SAMPLE_RATE_F;
    return audio;
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

// --- Playback ---

SolAudioHandle Sol_Audio_Play(SolAudioId id, float volume, float seek, u32 concurrent)
{
    SolAudioHandle handle = Sol_Audio_Alloc(id, false, volume, concurrent);
    if (!Sol_Audio_IsHandleValid(handle))
        return INVALID_AUDIO_HANDLE;

    PlayingSound *ps = &playing_pool[handle.index];
    if (seek > 0)
        ma_sound_seek_to_pcm_frame(&ps->sound, SOL_SECONDS_TO_FRAMES(seek));

    ma_sound_start(&ps->sound);
    return handle;
}

SolAudioHandle Sol_Audio_PlayAt(SolAudioId id, vec3s pos, float volume, float seek, u32 concurrent)
{
    SolAudioHandle handle = Sol_Audio_Alloc(id, true, volume, concurrent);
    if (!Sol_Audio_IsHandleValid(handle))
        return INVALID_AUDIO_HANDLE;

    PlayingSound *ps = &playing_pool[handle.index];
    ma_sound_set_position(&ps->sound, pos.x, pos.y, pos.z);
    if (seek > 0)
        ma_sound_seek_to_pcm_frame(&ps->sound, SOL_SECONDS_TO_FRAMES(seek));

    ma_sound_start(&ps->sound);
    return handle;
}

// --- Handle mutation ---

void Sol_Audio_SetVolume(float volume)
{
    ma_engine_set_volume(&audio_engine, volume);
}

void Sol_Audio_SetSlotPosition(SolAudioHandle handle, vec3s pos)
{
    if (!Sol_Audio_IsHandleValid(handle))
        return;
    ma_sound_set_position(&playing_pool[handle.index].sound, pos.x, pos.y, pos.z);
}

void Sol_Audio_SetSlotVolume(SolAudioHandle handle, float volume)
{
    if (!Sol_Audio_IsHandleValid(handle))
        return;
    ma_sound_set_volume(&playing_pool[handle.index].sound, volume);
}

void Sol_Audio_SetSlotPitch(SolAudioHandle handle, float pitch)
{
    if (!Sol_Audio_IsHandleValid(handle))
        return;
    ma_sound_set_pitch(&playing_pool[handle.index].sound, pitch);
}

void Sol_Audio_SetSlotLooping(SolAudioHandle handle, bool loop)
{
    if (!Sol_Audio_IsHandleValid(handle))
        return;
    ma_sound_set_looping(&playing_pool[handle.index].sound, loop ? MA_TRUE : MA_FALSE);
}

void Sol_Audio_StopSlot(SolAudioHandle handle)
{
    if (!Sol_Audio_IsHandleValid(handle))
        return;
    Slot_Uninit(&playing_pool[handle.index]);
}