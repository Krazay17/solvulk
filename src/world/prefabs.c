#include "sol_core.h"

int Sol_Prefab_Wizard(World *world, vec3s pos)
{
    int id = Entity_Create(world);
    Entity_Add_Xform(world, id, (CompXform){.pos = pos});
    Entity_Add_Model(world, id, (CompModel){.gpuHandle = Sol_Loader_GetBank()->models.wizard});
    Entity_Add_Controller_Local(world, id, (CompController){0});
    
    Entity_Add_Body3(world, id, (CompBody){.height = 1, .width = 0.5f, .mass = 1});
    return id;
}

int Sol_Prefab_Button(World *world, vec3s pos)
{
    int id = Entity_Create(world);
    Entity_Add_Xform(world, id, (CompXform){.pos = pos});
    Entity_Add_Shape(world, id, (CompShape){.type = SHAPE_RECTANGLE, .height = 50, .width = 150});
    Entity_Add_Interact(world, id, (CompInteractable){0});
    Entity_Add_UiElement(world, id, (CompUiElement){
                                        .baseColor = {255, 0, 0, 255},
                                        .borderColor = {0, 0, 0, 255},
                                        .text = "Button",
                                        .fontSize = 16.0f,
                                        .textColor = {0, 255, 0, 255},
                                        .borderThickness = 2.0f,
                                    });
    return id;
}

int Sol_Prefab_Boxman(World *world, vec3s pos)
{
    float width = 50.0f;
    float height = 50.0f;
    int id = Entity_Create(world);
    Entity_Add_Xform(world, id, (CompXform){.pos = pos});
    Entity_Add_Shape(world, id, (CompShape){.type = SHAPE_RECTANGLE, .height = height, .width = width});
    Entity_Add_Body2(world, id, (CompBody){
                                   .width = width,
                                   .height = height,
                               });
    Entity_Add_Controller_Local(world, id, (CompController){0});
    Entity_Add_Movement(world, id, (CompMovement){
                                       .gSpeed = 1.0f,
                                       .gAccell = 10.0f,
                                       .gFriction = 0.1f,
                                       .aSpeed = 1.0f,
                                       .aAccell = 1.0f,
                                       .aFriction = 0.1f,
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
