#pragma once

#include "base.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

// Global handle to the dynamic library
extern HMODULE current_engine_lib;

// ---------------------------------------------------------------------------
// Helper: Check DLL modification time safely
// ---------------------------------------------------------------------------
static inline FILETIME get_last_write_time(const char *path)
{
    FILETIME                  lastWriteTime = {0};
    WIN32_FILE_ATTRIBUTE_DATA data;
    
    // Explicitly use GetFileAttributesExA for const char*
    if (GetFileAttributesExA(path, GetFileExInfoStandard, &data))
    {
        lastWriteTime = data.ftLastWriteTime;
    }
    return lastWriteTime;
}

// ---------------------------------------------------------------------------
// Helper: Safely hot-reload the API
// ---------------------------------------------------------------------------
static inline bool load_api(const char *path)
{
    // 1. Check if the source file is actually readable (prevents copying while linker is writing)
    WIN32_FILE_ATTRIBUTE_DATA data;
    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &data))
    {
        return false; // Compiler hasn't finished writing yet
    }

    // 2. Unload the currently running engine DLL
    if (current_engine_lib)
    {
        FreeLibrary(current_engine_lib);
        current_engine_lib = NULL;
    }

    // 3. Copy the compiled DLL to a live workspace DLL using CopyFileA (ANSI)
    // FALSE = allow overwriting the destination file
    if (!CopyFileA(path, "solvulk_live.dll", FALSE))
    {
        // Copy failed (likely because compiler still has a lock on path)
        return false; 
    }

    // 4. Load the copied DLL
    current_engine_lib = LoadLibraryA("solvulk_live.dll");
    if (!current_engine_lib)
    {
        printf("[HOT RELOAD] Failed to load solvulk_live.dll! Error Code: %lu\n", GetLastError());
        return false;
    }

    // 5. Map function pointers from functions.inc
#define SOL_FUNC(ret, name, ...) \
    pfn_##name = (name##_fn)GetProcAddress(current_engine_lib, #name);
#include "functions.inc"
#undef SOL_FUNC

    return true;
}