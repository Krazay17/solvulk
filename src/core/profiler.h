#pragma once

#include <stdio.h>
#include <stdbool.h>

#ifdef WIN32
#include <Windows.h>
#define LONGINT LARGE_INTEGER
#else
#include <time.h>
#define LONGINT long long
#endif

typedef struct
{
    const char *name;
    LONGINT     start;
    double      totalMs;
    int         count;
    int         limiter;
    double      accumulator;
} SolProfiler;

static LONGINT _profFreq;
static int     _profFreqInit = 0;

static inline void Prof_Begin(SolProfiler *p)
{
#ifdef WIN32
    if (!_profFreqInit)
    {
        QueryPerformanceFrequency(&_profFreq);
        _profFreqInit = 1;
    }
    QueryPerformanceCounter(&p->start);
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    p->start = (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
#endif
}

static inline void Prof_End(SolProfiler *p)
{
#ifdef WIN32
    LARGE_INTEGER end;
    QueryPerformanceCounter(&end);
    double ms = (double)(end.QuadPart - p->start.QuadPart) / (double)_profFreq.QuadPart * 1000.0;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    long long end = (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
    double    ms  = (double)(end - p->start) / 1000000.0;
#endif

    p->totalMs += ms;
    p->count++;
}

static inline void Prof_Print(SolProfiler *p)
{
    if (p->count > 0)
    {
        printf("%-24s avg=%.3fms total=%.1fms calls=%d\n", p->name, p->totalMs / p->count, p->totalMs, p->count);
    }
}

static inline void Prof_Reset(SolProfiler *p)
{
    p->totalMs = 0;
    p->count   = 0;
}

static inline void Prof_EndEz(SolProfiler *p, bool onTick, double dt)
{
    Prof_End(p);
    p->accumulator += dt;
    if (onTick && p->accumulator < 2.0f)
        return;
    p->accumulator = 0;

    Prof_Print(p);
    Prof_Reset(p);
}