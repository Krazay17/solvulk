#include "sol_core.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

SolBank bank = {0};

SolBank *Sol_Load_Resources()
{
    for (int i = 0; i < SOL_MODEL_COUNT; i++)
    {
        SolResource res = Sol_LoadResource(model_path[i]);
        if (res.data)
        {
            bank.models[i] = Parse_Model(res);
            Sol_UploadModel(&bank.models[i], i);
            if (res.isHeap)
                free(res.data);
        }
    }

    for (int i = 0; i < SOL_FONT_COUNT; i++)
    {
        SolResource fontIceMetrics = Sol_LoadResource(fontResourceName[i][0]);
        SolResource fontIceAtlas   = Sol_LoadResource(fontResourceName[i][1]);
        if (!fontIceAtlas.data || !fontIceMetrics.data)
            continue;

        Load_Font(&bank.fonts[SOL_FONT_ICE], fontIceMetrics.data, fontIceAtlas.data, 224.0f, 224.0f);
        Sol_UploadImage(fontIceAtlas.data, 224, 224, VK_FORMAT_R8G8B8A8_UNORM, SOL_IMAGE_FONT);
    }

    return &bank;
}

Sol_Free_Resource(SolResource *res)
{
    if (res->isHeap)
    {
        free(res->data);
    }
}

SolBank *Sol_Getbank()
{
    return &bank;
}

SolModel *Sol_GetModel(SolModelId id)
{
    return &bank.models[id];
}

int Sol_ReadFile(const char *filename, SolResource *outRes)
{
    FILE *file = fopen(filename, "rb");

    if (file)
    {
        fseek(file, 0, SEEK_END);
        outRes->size = ftell(file);
        rewind(file);

        outRes->data = malloc(outRes->size);
        fread(outRes->data, 1, outRes->size, file);
        fclose(file);
        outRes->isHeap = 1;
        return 1;
    }
    else
        return 0;
}

SolResource Sol_LoadResource(const char *resourceName)
{
    SolResource res = {0};

    FILE *file = fopen(resourceName, "rb");
    if (file)
    {
        fseek(file, 0, SEEK_END);
        res.size = ftell(file);
        rewind(file);

        res.data = malloc(res.size);
        fread(res.data, 1, res.size, file);
        fclose(file);
        res.isHeap = 1;
        return res;
    }

    HMODULE hmod = NULL;

    GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                      (LPCTSTR)Sol_LoadResource, &hmod);

    HRSRC hResource = FindResource(hmod, resourceName, RT_RCDATA);
    if (!hResource && hmod != NULL)
    {
        hResource = FindResource(NULL, resourceName, RT_RCDATA);
        hmod      = NULL; // Reset hmod so LoadResource/SizeofResource use the EXE
    }
    if (!hResource)
    {
        printf("Failed to find resource %s\n", resourceName);
        return res;
    }

    HGLOBAL hLoaded = LoadResource(hmod, hResource);
    if (!hLoaded)
    {
        printf("Failed to load resource %s\n", resourceName);
        return res;
    }

    res.data = LockResource(hLoaded);
    res.size = SizeofResource(hmod, hResource);

    return res;
}
