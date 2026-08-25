#pragma once
#include "s_model.h"

#define BLEND_SPEED_DEFAULT 0.2f

typedef struct
{
    int        cnt, cap;
    int       *sparse, *dense;
    CompModel *models;

    int       anim_cnt, anim_cap;
    int      *anim_sparse, *anim_dense;
    CompAnim *anims;
} WorldModels;

void Anim_Tick(World *world, double dt, double time);
void Anim_Solver(World *world, double dt, double time);

CompAnim *Sol_Model_AddAnim(WorldModels *wc, int id, int kind);
CompAnim *Sol_Model_GetAnim(World *world, int id);
void      Sol_Model_RemAnim(WorldModels *wc, int id);
