#include "sol_core.h"

typedef struct CompAudio
{
    uint32_t bufferId; // The actual sound data
    uint32_t sourceId; // The active hardware/API source
    float    volume;
    float    pitch;
    bool     isLooping;
    bool     is3D;
} CompAudio;

void Sol_World_Audio_Init(World *world)
{
    u32 idx = world->stepCount++;
    world->stepSystems[idx] = Sol_World_Audio_Step;
}

void Sol_World_Audio_Add(World *world, int id, AudioDesc desc)
{
    world->masks[id] |= HAS_AUDIO;
    CompAudio *audio = &world->audios[id];
    audio->isLooping = desc.doesLoop;
    audio->is3D = desc.is3D;
}

void Sol_World_Audio_Step(World *world, double dt, double time)
{
    int required = HAS_AUDIO;
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;
        CompAudio *audio = &world->audios[id];

    }
}