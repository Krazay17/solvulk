#pragma once

#include <stdio.h>
#include <windows.h>

typedef struct
{
    const char   *name;
    LARGE_INTEGER start;
    double        totalMs;
    int           count;
    int           limiter;
} SolProfiler;

static LARGE_INTEGER _profFreq;
static int           _profFreqInit = 0;

static inline void Prof_Begin(SolProfiler *p)
{
    if (!_profFreqInit)
    {
        QueryPerformanceFrequency(&_profFreq);
        _profFreqInit = 1;
    }
    QueryPerformanceCounter(&p->start);
}

static inline void Prof_End(SolProfiler *p)
{
    LARGE_INTEGER end;
    QueryPerformanceCounter(&end);
    double ms = (double)(end.QuadPart - p->start.QuadPart) / _profFreq.QuadPart * 1000.0;
    p->totalMs += ms;
    p->count++;
}

static inline void Prof_Print(SolProfiler *p)
{
    if (p->count > 0)
        printf("%-24s avg=%.3fms total=%.1fms calls=%d\n", p->name, p->totalMs / p->count, p->totalMs, p->count);
}

static inline void Prof_Reset(SolProfiler *p)
{
    p->totalMs = 0;
    p->count   = 0;
}

static inline void Prof_EndEz(SolProfiler *p, bool onTick)
{
    Prof_End(p);
    if (onTick && p->limiter++ % 500 != 0)
        return;

    Prof_Print(p);
    Prof_Reset(p);
}