#include "sol_core.h"

int Sol_Prefab_Floor(World *world, vec3s pos)
{
    int id = Sol_Create_Ent(world);

    Sol_Xform_Add(world, id, (vec3s){0, 0, 0});
    Sol_Model_Add(world, id, (ModelDesc){.id = SOL_MODEL_WORLD1});
    Sol_Body_Add(world, id, (BodyDesc){.shape = SHAPE3_MOD});

    // Sol_Interact_Add(world, id);

    return id;
}

int Sol_Prefab_Box(World *world, vec3s pos)
{

    int id = Sol_Create_Ent(world);
    Sol_Xform_Add(world, id, pos);
    Sol_Model_Add(world, id, (ModelDesc){.id = SOL_MODEL_BOX});
    Sol_Body_Add(world, id, (BodyDesc){.mass = 0, .radius = 1.0f, .shape = SHAPE3_MOD, .group = 0b01});
    Sol_Interact_Add(world, id, (InteractDesc){0});
    Sol_Flags_Add(world, id, EFLAG_PICKUPABLE);

    return id;
}

int Sol_Prefab_Ball(World *world, vec3s pos, vec3s vel, SphereDesc desc)
{
    int id = Sol_Create_Ent(world);
    Sol_Sphere_Add(world, id, desc);
    Sol_Xform_Add(world, id, pos);
    Sol_Body_Add(world, id,
                 (BodyDesc){
                     .radius      = desc.radius,
                     .shape       = SHAPE3_SPH,
                     .mass        = 1.0f * desc.radius,
                     .restitution = 0.5f,
                     .vel         = vel,
                     .group       = 0b10,
                 });
    Sol_Interact_Add(world, id, (InteractDesc){0});
    Sol_Flags_Add(world, id, EFLAG_PICKUPABLE);

    return id;
}

int Sol_Prefab_Pawn(World *world, vec3s pos, SolModelId modelid, float height)
{
    int id = Sol_Create_Ent(world);

    Sol_Xform_Add(world, id, pos);
    Sol_Body_Add(world, id,
                 (BodyDesc){
                     .height      = height,
                     .radius      = 0.5f,
                     .mass        = 1.0f,
                     .shape       = SHAPE3_CAP,
                     .restitution = 0.1f,
                     .group       = 1,
                 });

    Sol_Movement_Add(world, id, (MovementDesc){.configId = MOVE_CONFIG_PLAYER});
    Sol_Model_Add(world, id, (ModelDesc){.id = modelid, .yoffset = -height * 0.5f});

    Sol_Ability_Add(world, id, (AbilityDesc){0});
    Sol_Interact_Add(world, id, (InteractDesc){0});
    Sol_Flags_Add(world, id, EFLAG_PICKUPABLE);

    return id;
}

int Sol_Prefab_Button(World *world, vec3s pos, const char *text)
{
    float width  = 150.0f;
    float height = 50.0f;
    int   id     = Sol_Create_Ent(world);

    Sol_Xform_Add(world, id, pos);
    Sol_Body_Add(world, id, (BodyDesc){.height = 50, .radius = 150, .is2d = true});
    Sol_Interact_Add(world, id, (InteractDesc){0});

    Sol_Ui_Add(world, id,
               (UiDesc){
                   .kind            = UI_BUTTON,
                   .baseColor       = (vec4s){255, 0, 0, 255},
                   .borderColor     = (vec4s){0, 0, 0, 255},
                   .textColor       = (vec4s){0, 255, 0, 255},
                   .borderThickness = 2.0f,
                   .fontSize        = 16.0f,
                   .textWidth       = Sol_MeasureText(text, 16.0f, SOL_FONT_ICE),
                   .text            = text,
               });

    return id;
}

int Sol_Prefab_Slider(World *world, vec3s pos, const char *text)
{
    int id = Sol_Create_Ent(world);
    Sol_Xform_Add(world, id, pos);
    Sol_Ui_Add(world, id, (UiDesc){.kind = UI_SLIDER});
    // v->baseColor       = (vec4s){255, 0, 0, 255};
    // v->borderColor     = (vec4s){0, 0, 0, 255};
    // v->textColor       = (vec4s){0, 255, 0, 255};
    // v->borderThickness = 2.0f;
    // v->fontSize        = 16.0f;
    // v->textWidth       = Sol_MeasureText(text, 16.0f, SOL_FONT_ICE);
    // strncpy_s(v->text, sizeof(v->text), text, 64);

    return id;
}
