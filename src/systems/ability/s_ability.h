/*
 * File: s_ability.h
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-10-07
 *
 */
#include "sol/types.h"
#include "components.h"

extern const u32 slot_kind_map[7];

static inline HitPause(AbilityStateData *data)
{
    if (data->hitPauseDr < 4)
    {
        data->hitPause = data->hitPauseDr > 0 ? 1.0f / data->hitPauseDr : 1.0f;
        data->hitPauseDr++;
    }
}

typedef struct
{
    int stage;
    bool advanced;
    bool finished;
} StageProgress;
static inline StageProgress Progress_Stage(AbilityStateData *data, float dt, const float *stage_time, int stage_count)
{
    StageProgress prog = {0};
    data->elapsed += dt;
    if (data->hitPause > 0)
        data->hitPause = fmaxf(0, data->hitPause - dt * 8.0f);
    else
        data->accum += dt;
    while (data->stage < stage_count && data->accum >= stage_time[data->stage])
    {
        data->accum -= stage_time[data->stage];
        data->stage++;

        prog.advanced = true;
    }
    prog.stage    = data->stage;
    prog.finished = prog.stage >= stage_count;

    return prog;
}
