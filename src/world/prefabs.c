#include "sol_core.h"

int Sol_Prefab_Wizard(World *world, vec3s pos)
{
    int id = Entity_Create(world);
    Entity_Add_Xform(world, id, (CompXform){.pos = pos, .rot = (vec4s){
                                                            0.0f,
                                                            0.0f,
                                                            0.0f,
                                                            1.0f,
                                                        }});
    Entity_Add_Model(world, id, (CompModel){.gpuHandle = SOL_MODEL_WIZARD});
    Entity_Add_Movement(world, id, (CompMovement){.configId = MOVE_CONFIG_PLAYER});
    Entity_Add_Body3(world, id, (CompBody){.height = 1, .radius = 0.5f, .mass = 1, .type = BODY_DYNAMIC, .restitution = 0.8});
    return id;
}

int Sol_Prefab_Button(World *world, vec3s pos, const char *text)
{
    float width = 150.0f;
    float height = 50.0f;
    int id = Entity_Create(world);
    Entity_Add_Xform(world, id, (CompXform){.pos = pos});
    Entity_Add_Shape(world, id, (CompShape){.type = SHAPE_RECTANGLE, .width = width, .height = height});

    CompUiElement compElement = {
        .baseColor = {255, 0, 0, 255},
        .borderColor = {0, 0, 0, 255},
        .fontSize = 16.0f,
        .textColor = {0, 255, 0, 255},
        .borderThickness = 2.0f,
        .textWidth = Sol_MeasureText(text, 16.0f),
    };
    strncpy_s(compElement.text, sizeof(compElement.text), text, 64);
    Entity_Add_UiElement(world, id, compElement);

    return id;
}

int Sol_Prefab_Boxman(World *world, vec3s pos)
{
    float radius = 50.0f;
    float height = 50.0f;
    int id = Entity_Create(world);
    Entity_Add_Xform(world, id, (CompXform){.pos = pos});
    Entity_Add_Shape(world, id, (CompShape){.type = SHAPE_RECTANGLE, .height = height, .width = radius});
    Entity_Add_Body2(world, id, (CompBody){
                                    .radius = radius,
                                    .height = height,
                                });
    Entity_Add_Movement(world, id, (CompMovement){
                                       .configId = MOVE_CONFIG_PLAYER,
                                   });
    Entity_Add_UiElement(world, id, (CompUiElement){
                                        .baseColor = {255, 0, 0, 255},
                                        .borderColor = {0, 255, 0, 255},
                                        .fontSize = 16.0f,
                                        .textColor = {0, 255, 0, 255},
                                        .borderThickness = 2.0f,
                                    });
    return id;
}
