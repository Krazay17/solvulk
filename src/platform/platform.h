#pragma once

typedef struct SolResource
{
    void *data;
    long  size;
    int   isHeap;
} SolResource;

SolResource Sol_LoadResource(const char *resourceName);
int         Sol_ReadFile(const char *filename, SolResource *outRes);
void        Sol_Platform_LockCursor(bool lock);
void        Sol_Platform_SetCursorpos(int x, int y);
void        Sol_MessageBox(const char *text, const char *level);
