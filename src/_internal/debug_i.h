#pragma once

#define MAX_DEBUGS 14
#define MAX_STR_LEN 64

typedef struct Debuggers
{
    int characterCount[MAX_DEBUGS];
    char text[MAX_DEBUGS][MAX_STR_LEN];
    float value[MAX_DEBUGS];
    int count;
} Debuggers;

void Sol_Debug_Draw(double dt);