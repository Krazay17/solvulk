#include "sol_core.h"

int Sol_Prefab_Wizard(World *world, vec3s pos)
{
    float height = 2.0f;
    int id = Entity_Create(world);

    CompXform *xform = Entity_Add_Xform(world, id, pos);

    CompBody *body = Entity_Add_Body3(world, id);
    body->height = height;
    body->radius = 0.5f;
    body->mass = 1.0f;
    body->type = BODY_DYNAMIC;
    body->shape = BODY_SHAPE_CAPSULE;
    body->restitution = 0.8f;
    body->invMass = 1.0f / body->mass;
    
    CompMovement *movement = Entity_Add_Movement(world, id);
    movement->configId = MOVE_CONFIG_PLAYER;
    CompModel *model = Entity_Add_Model(world, id, SOL_MODEL_WORLD0);
    model->gpuHandle = SOL_MODEL_WIZARD;
    model->yOffset = -height * 0.5f;

    return id;
}

int Sol_Prefab_Button(World *world, vec3s pos, const char *text)
{
    float width = 150.0f;
    float height = 50.0f;
    int id = Entity_Create(world);

    CompXform *xform = Entity_Add_Xform(world, id, pos);

    CompShape *shape = Entity_Add_Shape(world, id);
    shape->type = SHAPE_RECTANGLE;
    shape->width = width;
    shape->height = height;

    CompUiElement *uiEle = Entity_Add_UiElement(world, id);
    uiEle->baseColor = (SolColor){255, 0, 0, 255};
    uiEle->borderColor = (SolColor){0, 0, 0, 255};
    uiEle->textColor = (SolColor){0, 255, 0, 255};
    uiEle->borderThickness = 2.0f;
    uiEle->fontSize = 16.0f;
    uiEle->textWidth = Sol_MeasureText(text, 16.0f);
    strncpy_s(uiEle->text, sizeof(uiEle->text), text, 64);

    return id;
}

int Sol_Prefab_Boxman(World *world, vec3s pos)
{
    float radius = 50.0f;
    float height = 50.0f;
    int id = Entity_Create(world);
    // Entity_Add_Xform(world, id, (CompXform){.pos = pos});
    // Entity_Add_Shape(world, id, (CompShape){.type = SHAPE_RECTANGLE, .height = height, .width = radius});
    // Entity_Add_Body2(world, id, (CompBody){
    //                                 .radius = radius,
    //                                 .height = height,
    //                             });
    // Entity_Add_Movement(world, id, (CompMovement){
    //                                    .configId = MOVE_CONFIG_PLAYER,
    //                                });
    // Entity_Add_UiElement(world, id, (CompUiElement){
    //                                     .baseColor = {255, 0, 0, 255},
    //                                     .borderColor = {0, 255, 0, 255},
    //                                     .fontSize = 16.0f,
    //                                     .textColor = {0, 255, 0, 255},
    //                                     .borderThickness = 2.0f,
    //                                 });
    return id;
}
