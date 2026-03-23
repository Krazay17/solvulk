#pragma once

typedef int SolModelHandle;

typedef struct
{
    SolModelHandle wizard;
} BankedModels;

typedef struct SolBank
{
    BankedModels models;
} SolBank;