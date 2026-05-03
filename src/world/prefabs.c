#include "sol_core.h"

int Sol_Prefab_Floor(World *world, vec3s pos)
{
    int id = Sol_Create_Ent(world);

    CompXform *floorXform = Sol_Xform_Add(world, id, (vec3s){0, 0, 0});
    CompModel *floorModel = Sol_Model_Add(world, id, (CompModel){.modelId = SOL_MODEL_WORLD1});
    CompBody  *floorBody  = Sol_Body_Add(world, id, (CompBody){.shape = SHAPE3_MOD});

    // Sol_Interact_Add(world, id);

    return id;
}

int Sol_Prefab_Box(World *world, vec3s pos)
{

    int id = Sol_Create_Ent(world);
    Sol_Xform_Add(world, id, pos);
    Sol_Model_Add(world, id, (CompModel){.modelId = SOL_MODEL_BOX});
    Sol_Body_Add(world, id, (CompBody){.mass = 0, .radius = 1.0f, .shape = SHAPE3_MOD, .group = 0b01});
    Sol_Interact_Add(world, id);
    Sol_Flags_Add(world, id, EFLAG_PICKUPABLE);

    return id;
}
int Sol_Prefab_Ball(World *world, vec3s pos, vec3s vel, CompSphere sphere)
{
    int id = Sol_Create_Ent(world);
    Sol_Sphere_Add(world, id, sphere);
    Sol_Xform_Add(world, id, pos);
    // Sol_Timer_Add(world, id, (CompTimer){.duration = 55.0f});
    Sol_Body_Add(world, id,
                 (CompBody){
                     .radius      = sphere.radius,
                     .shape       = SHAPE3_SPH,
                     .mass        = 1.0f * sphere.radius,
                     .restitution = 0.5f,
                     .vel         = vel,
                     .group       = 0b10,
                 });
    Sol_Interact_Add(world, id);
    Sol_Flags_Add(world, id, EFLAG_PICKUPABLE);

    return id;
}

int Sol_Prefab_Pawn(World *world, vec3s pos, SolModelId modelId, float height)
{
    int   id     = Sol_Create_Ent(world);

    CompXform *xform = Sol_Xform_Add(world, id, pos);

    Sol_Body_Add(world, id,
                 (CompBody){
                     .height      = height,
                     .radius      = 0.5f,
                     .mass        = 1.0f,
                     .shape       = SHAPE3_CAP,
                     .restitution = 0.1f,
                     .group       = 1,
                 });

    CompMovement *movement = Sol_Movement_Add(world, id, (CompMovement){.configId = MOVE_CONFIG_PLAYER});
    CompModel    *model    = Sol_Model_Add(world, id, (CompModel){.modelId = modelId});
    model->yOffset         = -height * 0.5f;

    CompAbility *combat = Sol_Ability_Add(world, id, (CompAbility){0});
    Sol_Interact_Add(world, id);
    Sol_Flags_Add(world, id, EFLAG_PICKUPABLE);

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

    CompUiView *uiEle      = Sol_UiView_Add(world, id);
    uiEle->baseColor       = (vec4s){255, 0, 0, 255};
    uiEle->borderColor     = (vec4s){0, 0, 0, 255};
    uiEle->textColor       = (vec4s){0, 255, 0, 255};
    uiEle->borderThickness = 2.0f;
    uiEle->fontSize        = 16.0f;
    uiEle->textWidth       = Sol_MeasureText(text, 16.0f, SOL_FONT_ICE);
    strncpy_s(uiEle->text, sizeof(uiEle->text), text, 64);

    return id;
}

int Sol_Prefab_Slider(World *world, vec3s pos, const char *text)
{
    int id = Sol_Create_Ent(world);
    Sol_Xform_Add(world, id, pos);
    CompShape *s = Sol_Shape_Add(world, id);
    s->height    = 50.0f;
    s->width     = 100.0f;
    s->type      = SHAPE2_REC;

    // CompUiSlider *slider = Sol_Slider_Add

    CompUiView *v      = Sol_UiView_Add(world, id);
    v->baseColor       = (vec4s){255, 0, 0, 255};
    v->borderColor     = (vec4s){0, 0, 0, 255};
    v->textColor       = (vec4s){0, 255, 0, 255};
    v->borderThickness = 2.0f;
    v->fontSize        = 16.0f;
    v->textWidth       = Sol_MeasureText(text, 16.0f, SOL_FONT_ICE);
    strncpy_s(v->text, sizeof(v->text), text, 64);

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
    // Sol_UiView_Add(world, id, (CompUiView){
    //                                     .baseColor = {255, 0, 0, 255},
    //                                     .borderColor = {0, 255, 0, 255},
    //                                     .fontSize = 16.0f,
    //                                     .textColor = {0, 255, 0, 255},
    //                                     .borderThickness = 2.0f,
    //                                 });
    return id;
}
