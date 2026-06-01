#include "sol_core.h"

#include "network.h"

SolState solState = {0};

static double accumulator = 0.0;
static void   DebugFPS(double dt);
static void   Sol_OnResize();

int Sol_Init(void *hwnd, void *hInstance)
{
    solState.timescale = 1.0;
    solState.g_hwnd    = hwnd;
    i32 res;

    res = Sol_Audio_Init();
    if (res != 0)
        printf("Audio failed to init, code:%d\n", res);
    res = Sol_Audio_LoadAll();
    if (res != 0)
        printf("Audio failed to load, code:%d\n", res);
    res = Sol_Textures_Init();
    if (res != 0)
        printf("Textures failed to init, code:%d\n", res);
    res = Sol_Fonts_Init();
    if (res != 0)
        printf("Fonts failed to init, code:%d\n", res);
    res = Sol_Models_Init();
    if (res != 0)
        printf("Models failed to init, code:%d\n", res);
    res = Sol_Render_Init(hwnd, hInstance);
    if (res != 0)
        printf("Render failed to init, code:%d\n", res);

    solState.debug     = true;
    solState.isRunning = true;
    return res;
}

World *Sol_GetWorldById(u32 id)
{
    for (int i = 0; i < solState.worldCount; i++)
    {
        if (solState.worlds[i]->worldId == id)
            return solState.worlds[i];
    }
    return NULL;
}

SolState *Sol_GetState()
{
    return &solState;
}

double Sol_GetGameTime()
{
    return solState.gameTime;
}

void Sol_Destroy()
{
    Net_DeInit();

    for (int i = 0; i < solState.worldCount; i++)
    {
        free(solState.worlds[i]);
    }
    solState.isRunning = false;
}

void Sol_Window_Resize(float width, float height)
{
    solState.windowWidth  = width;
    solState.windowHeight = height;
    solState.needsResize  = true;
}

static void Sol_OnResize()
{
    solState.needsResize = false;
    if (solState.windowWidth != 0 && solState.windowHeight != 0)
    {
        Sol_Render_Resize(solState.windowWidth, solState.windowHeight);
    }
}

void Sol_State_SetTimescale(float timescale)
{
    solState.timescale = timescale;
}

void Sol_Tick(double dt, double time)
{
    solState.gameTime += dt;
    solState.tickCounter++;
    Sol_Input_Update();
    if (Sol_Input_KeyPressed(SOL_KEY_ESCAPE))
    {
        bool menuActive = solState.worlds[0]->doesSimulate;
        menuActive ^= 1;
        solState.worlds[0]->doesSimulate = menuActive;
        solState.worlds[0]->doesRender   = menuActive;
        Sol_Input_SetLocked(!menuActive);
    }

    if (solState.needsResize)
        Sol_OnResize();

    if (Net_IsActive())
        Net_Poll();

    if (Net_IsPlaying() && Net_IsClient())
    {
        for (int i = 0; i < solState.worldCount; i++)
        {
            World *world = solState.worlds[i];
            if (world->doesReplicate)
                Net_Apply_Snap(solState.worlds[i]);
        }
    }

    for (int i = 0; i < solState.worldCount; ++i)
    {
        if (!solState.worlds[i]->doesSimulate)
            continue;
        World_Tick(solState.worlds[i], dt, time);
    }

    // ######### STEP AND INTERP #########
    accumulator = accumulator > SOL_TIMESTEP * 10.0 ? SOL_TIMESTEP * 10.0 : accumulator + dt;
    for (int i = 0; i < solState.worldCount; ++i)
    {
        if (!solState.worlds[i]->doesSimulate)
            continue;
        if (accumulator >= SOL_TIMESTEP)
            Xform_Snapshot(solState.worlds[i]);
    }
    while (accumulator >= SOL_TIMESTEP)
    {
        for (int i = 0; i < solState.worldCount; ++i)
        {
            World *world = solState.worlds[i];
            if (!world->doesSimulate)
                continue;
            World_Step(world, SOL_TIMESTEP, time);

            if (Net_IsPlaying() && world->doesReplicate)
            {
                if (Net_IsClient())
                {
                    Net_Send_Input(world);
                }
                else if (Net_IsHost() && Net_ShouldSend_Snap())
                {
                    Net_Send_Snap(world);
                    Net_Send_Events(world);
                }
            }
            Sol_Event_Clear(world);
        }
        Net_Heartbeat(time);
        solState.stepCounter++;
        accumulator -= SOL_TIMESTEP;
    }
    // Post Step
    float alpha = (float)(accumulator / SOL_TIMESTEP);
    for (int i = 0; i < solState.worldCount; ++i)
    {
        World *world = solState.worlds[i];
        if (!world->doesSimulate)
            continue;
        Xform_Interpolate(world, alpha);
    }
    // ######### END STEP AND INTERP #########

    if (solState.activeWorld)
        Sol_Audio_Update(Sol_Xform_GetPos(solState.activeWorld, 1), Sol_Controller_GetAimdir(solState.activeWorld, 1));

    Sol_Begin_Draw();

    Sol_Cam_Update(dt);
    Sol_Render_DrawSkybox();
    
    for (int i = solState.worldCount - 1; i >= 0; --i)
    {
        if (!solState.worlds[i]->doesRender)
            continue;
        World_Draw3d(solState.worlds[i], dt, time);
    }
    Sol_Render_Flush3D();

    for (int i = solState.worldCount - 1; i >= 0; --i)
    {
        if (!solState.worlds[i]->doesRender)
            continue;
        World_Draw2d(solState.worlds[i], dt, time);
    }
    Sol_Render_Flush2D();

    Sol_Debug_Draw(dt);
    Sol_End_Draw();
}

World *Sol_State_GetActiveWorld()
{
    return solState.activeWorld;
}