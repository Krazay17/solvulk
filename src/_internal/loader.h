#pragma once
#include "sol_core.h"

typedef struct
{
    void *data;
    long  size;
    int   isHeap;
} SolResource;

typedef struct
{
    SolFont  fonts[SOL_FONT_COUNT];
    SolImage images[SOL_IMAGE_COUNT];
    SolModel models[SOL_MODEL_COUNT];
} SolBank;

int Sol_ReadFile(const char *filename, SolResource *outRes);

SolBank *Sol_Load_Resources();
SolBank *Sol_Getbank();

SOLAPI SolResource Sol_LoadResource(const char *resourceName);
SolModel           Parse_Model(SolResource res);

void Load_Font(SolFont *font, const char *metrics, const char *atlas, float w, float h);
void Load_Model(SolModel *model, SolModelId id);

// name of the resource linked in the resources.rc
static const char *modelResourceName[SOL_MODEL_COUNT] = {
    "ID_MODEL_WIZARD",
    "ID_MODEL_WORLD0",
    "ID_MODEL_WORLD1",
    "ID_MODEL_WORLD2",
};

static const char *fontResourceName[SOL_FONT_COUNT][2] = {
    [SOL_FONT_ICE] = {"ID_FONT_METRICS", "ID_FONT_ATLAS"},
};

static const char *model_path[SOL_MODEL_COUNT] = {
    "Wizard.glb",
    "World0.glb",
    "World1.glb",
    "World2.glb",
};