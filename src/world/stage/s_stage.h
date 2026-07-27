#pragma once
#include "types.h"

typedef struct Stage
{
    bool           isLoaded;
    vec4s          scale;
    vec4s          pos;
    SolModelHandle handle;
} Stage;

void Sol_Stage_Init(World *world);

bool Sol_Stage_Load(World *world, Stage *stage, const char *path, vec3s offset);
void Sol_Stage_Unload(World *world, Stage *stage);