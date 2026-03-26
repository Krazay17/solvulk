#pragma once

typedef int SolModelHandle;

typedef struct
{
    SolModelHandle wizard;
    SolModelHandle world0;
} BankedModels;

typedef struct SolBank
{
    BankedModels models;
} SolBank;