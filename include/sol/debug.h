#pragma once
#include <string.h>

#define MAX_DEBUGS 12
#define MAX_STR_LEN 64

typedef struct
{
    int characterCount[MAX_DEBUGS];
    char text[MAX_DEBUGS][MAX_STR_LEN];
    float value[MAX_DEBUGS];
    int count;
} DebugLines;

void Sol_Debug_Add(const char *text, float value);
void Sol_Debug_Draw();