#pragma once

#ifdef SOL_VULK_SHARED
    #ifdef SOL_BUILD_DLL
        #define SOLAPI __declspec(dllexport)
    #else
        #define SOLAPI __declspec(dllimport)
    #endif
#else
    #define SOLAPI 
#endif