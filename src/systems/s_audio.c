#include "sol_core.h"

#define INIT_CAPACITY 64
#define MAX_AUDIOS 4

typedef struct CompAudio
{
    SolAudioHandle handle[MAX_AUDIOS];
    u32            count;
} CompAudio;

static void Audio_Step(World *world, double dt, double time);

void Sol_World_Audio_Init(World *world)
{
    world->audios   = calloc(MAX_ENTS, sizeof(CompAudio));
    WAddStep(world) = Audio_Step;
}

void Sol_World_Audio_Add(World *world, int id, u32 audioId, float volume, u32 slot)
{
    CompAudio *a = &world->audios[id];
    if (!(world->masks[id] & HAS_AUDIO))
        a->count = 0;
    world->masks[id] |= HAS_AUDIO;

    a->handle[slot] = Sol_Audio_PlayAt(audioId, Sol_Xform_GetPos(world, id), volume, 0, 32);
    a->count++;
}

void Sol_World_Audio_Remove(World *world, int id, u32 slot)
{
    Sol_Audio_StopSlot(world->audios[id].handle[slot]);
}

static void Audio_Step(World *world, double dt, double time)
{
    int required = HAS_AUDIO;
    for (int i = 0; i < world->activeCount; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;
        CompAudio *a     = &world->audios[id];
        CompXform *xform = &world->xforms[id];
        for (int i = 0; i < MAX_AUDIOS; i++)
        {
            if (a->handle[i].generation == 0)
                continue;
            Sol_Audio_SetSlotPosition(a->handle[i], xform->pos);
        }
    }
}