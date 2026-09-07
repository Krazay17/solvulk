/*
 * File: components.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-08-27
 *
 */

#include "components.h"
#include "world.h"

ScModel *Sol_Model_Add(World *world, int id, ModelKind kind)
{
    ScModel *model = Sol_Comp_Add(world, id, ScModel);
    model->kind     = kind;
    return model;
}

ScBody3 *Sol_Body3_Add(World *world, int id)
{
    ScBody3 *body3    = Sol_Comp_Add(world, id, ScBody3);
    body3->mass        = 1.0f;
    body3->invMass     = 1.0f;
    body3->restitution = 0.5f;
    body3->gravity     = SOL_PHYS_GRAV;

    return body3;
}

ScAnim *Sol_Anim_Add(World *world, int id)
{
    ScAnim *anim_comp = Sol_Comp_Add(world, id, ScAnim);
    for (int i = 0; i < ANIM_LAYER_COUNT; i++)
    {
        anim_comp->layers[i].currentAnim = -1;
        anim_comp->layers[i].animId      = -1;
        anim_comp->layers[i].currentSeek = 0.0f;
        anim_comp->layers[i].blendFactor = 1.0f;
        anim_comp->layers[i].weight      = 1.0f;
    }
    anim_comp->layers[0].animId      = 0;
    anim_comp->layers[0].currentAnim = 0;
    anim_comp->hasLastPose           = false;

    return anim_comp;
}