// NO PRAGMA ONCE HERE FOR HOT RELOAD FUNCTION POINTER/MAPPING GENERATOR
#ifndef SOL_FUNC
    // This tells Intellisense: "Don't panic, I'm just a list of functions"
    #define SOL_FUNC(ret, name, ...) ret name(__VA_ARGS__);
#endif
// Format: SOL_FUNC(return_type, function_name, args...)
SOL_FUNC(void, Sol_Init, void *hwnd, void *hInstance, SolConfig config)
SOL_FUNC(void, Sol_Tick, double dt, double time)
SOL_FUNC(void, Sol_Window_Resize, float width, float height)
SOL_FUNC(SolModel, Sol_LoadModel, const char* resourceName)