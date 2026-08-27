/*
 * File: components.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-08-27
 * 
*/

#include "components.h"
#include "world.h"

SolModel *Sol_Model_Add(World *world, int id, ModelKind kind)
{
    SolModel *model = Sol_Comp_Add(world, id, SolModel);
    *model          = model_kinds[kind];

    return model;
}

SolXform *Sol_Xform_Add(World *world, int id, vec3s pos)
{
    SolXform *xform = Sol_Comp_Add(world, id, SolXform);
    xform->rot      = (versors){0.0f, 0.0f, 0.0f, 1.0f};
    xform->last_rot = (versors){0.0f, 0.0f, 0.0f, 1.0f};
    xform->draw_rot = (versors){0.0f, 0.0f, 0.0f, 1.0f};
    xform->sca      = (vec3s){1.0f, 1.0f, 1.0f};
    xform->last_sca = (vec3s){1.0f, 1.0f, 1.0f};
    xform->draw_sca = (vec3s){1.0f, 1.0f, 1.0f};
    xform->pos      = pos;
    xform->last_pos = pos;
    xform->draw_pos = pos;

    return xform;
}

SolBody3 *Sol_Body3_Add(World *world, int id)
{
    SolBody3 *body3    = Sol_Comp_Add(world, id, SolBody3);
    body3->mass        = 1.0f;
    body3->invMass     = 1.0f;
    body3->restitution = 0.5f;
    body3->gravity     = SOL_PHYS_GRAV;
}

SolAnim *Sol_Anim_Add(World *world, int id)
{
    SolAnim *anim_comp = Sol_Comp_Add(world, id, SolAnim);
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