#include "sol_core.h"

int Sol_Prefab_Ball(World *world, vec3s pos, vec3s vel, CompSphere sphere)
{
    int   id     = Sol_Create_Ent(world);
    Sol_Sphere_Add(world, id, sphere);
    Sol_Xform_Add(world, id, pos);
    //Sol_Timer_Add(world, id, (CompTimer){.duration = 55.0f});
    Sol_Physx_Add(world, id,
                  (CompBody){
                      .radius      = sphere.radius,
                      .shape       = SHAPE3_SPH,
                      .mass        = 1.0f * sphere.radius,
                      .restitution = 0.5f,
                      .vel = vel,
                  });
    return id;
}

int Sol_Prefab_Wizard(World *world, vec3s pos)
{
    float height = 2.8f;
    int   id     = Sol_Create_Ent(world);

    CompXform *xform = Sol_Xform_Add(world, id, pos);

    Sol_Physx_Add(world, id,
                  (CompBody){
                      .height      = height,
                      .radius      = 0.5f,
                      .mass        = 1.0f,
                      .shape       = SHAPE3_CAP,
                      .restitution = 0.8f,
                  });

    CompMovement *movement = Sol_Movement_Add(world, id, (CompMovement){.configId = MOVE_CONFIG_PLAYER});
    CompModel    *model    = Sol_Model_Add(world, id, (CompModel){.modelId = SOL_MODEL_WIZARD});
    model->yOffset         = -height * 0.5f;

    CompCombat *combat = Sol_Combat_Add(world, id, (CompCombat){0});

    return id;
}

int Sol_Prefab_Button(World *world, vec3s pos, const char *text)
{
    float width  = 150.0f;
    float height = 50.0f;
    int   id     = Sol_Create_Ent(world);

    CompXform *xform = Sol_Xform_Add(world, id, pos);

    CompShape *shape = Sol_Shape_Add(world, id);
    shape->type      = SHAPE2_REC;
    shape->width     = width;
    shape->height    = height;

    CompUiElement *uiEle   = Sol_UiElement_Add(world, id);
    uiEle->baseColor       = (SolColor){255, 0, 0, 255};
    uiEle->borderColor     = (SolColor){0, 0, 0, 255};
    uiEle->textColor       = (SolColor){0, 255, 0, 255};
    uiEle->borderThickness = 2.0f;
    uiEle->fontSize        = 16.0f;
    uiEle->textWidth       = Sol_MeasureText(text, 16.0f, SOL_FONT_ICE);
    strncpy_s(uiEle->text, sizeof(uiEle->text), text, 64);

    return id;
}

int Sol_Prefab_Boxman(World *world, vec3s pos)
{
    float radius = 50.0f;
    float height = 50.0f;
    int   id     = Sol_Create_Ent(world);
    // Sol_Movement_Add(world, id, (CompXform){.pos = pos});
    // Sol_Shape_Add(world, id, (CompShape){.type = SHAPE_RECTANGLE, .height = height, .width = radius});
    // Sol_Body2_Add(world, id, (CompBody){
    //                                 .radius = radius,
    //                                 .height = height,
    //                             });
    // Sol_Movement_Add(world, id, (CompMovement){
    //                                    .configId = MOVE_CONFIG_PLAYER,
    //                                });
    // Sol_UiElement_Add(world, id, (CompUiElement){
    //                                     .baseColor = {255, 0, 0, 255},
    //                                     .borderColor = {0, 255, 0, 255},
    //                                     .fontSize = 16.0f,
    //                                     .textColor = {0, 255, 0, 255},
    //                                     .borderThickness = 2.0f,
    //                                 });
    return id;
}
