#include "sol/sol.h"

// static BuffDesc buffs[] = {
//     {.kind = BUFFKIND_KNOCKBACK, .duration = 0.5f},
//     {.kind = BUFFKIND_FIRE, .duration = 5.0f, .freq = 0.2f},
// };

static ImpactList impact_list = {
    .impacts[0] =
        {
            .kind = IMPACT_DIRECT,
            .hit =
                {
                    .damage = 25,
                    .kind   = DAMAGEKIND_FIRE,
                    .buffs =
                        {
                            {.kind = BUFFKIND_KNOCKBACK, .duration = 0.5f},
                            {.kind = BUFFKIND_FIRE, .duration = 5.0f, .freq = 0.2f},
                        },
                    .power     = 5.0f,
                    .buffcount = 2,
                },
        },
    .impacts[1] =
        {
            .kind = IMPACT_AOE,
            .hit =
                {
                    .damage = 25,
                    .kind   = DAMAGEKIND_FIRE,
                    .buffs =
                        {
                            {.kind = BUFFKIND_KNOCKBACK, .duration = 0.5f},
                            {.kind = BUFFKIND_FIRE, .duration = 5.0f, .freq = 0.2f},
                        },
                    .power     = 5.0f,
                    .buffcount = 2,
                },
            .radius = 5.0f,
        },

    .impactCount = 2,
};

int Sol_Prefab_Player(World *world, vec3s pos, float scale)
{
    vec2s dims = {.x = 0.35f, .y = 2.0f};

    dims   = glms_vec2_scale(dims, scale);
    int id = Sol_Create_Ent(world);
    Sol_Xform_Add(world, id, pos);
    Sol_Model_Add(world, id, (ModelDesc){.id = SOL_MODEL_DUDE, .yoffset = -dims.y * 0.5f, .yawOffset = GLM_PI_2f});
    Sol_Body_Add(world, id,
                 (BodyDesc){
                     .height      = dims.y,
                     .radius      = dims.x,
                     .mass        = 1.0f,
                     .shape       = SHAPE3_CAP,
                     .restitution = 0.1f,
                     .group       = 1,
                 });

    Sol_Movement_Add(world, id, (MovementDesc){.configId = MOVE_CONFIG_PLAYER});
    Sol_Vital_Add(world, id, (VitalDesc){.maxHealth = 100, .maxMana = 100, .maxEnergy = 100});
    Sol_Ability_Add(world, id, (AbilityDesc){0});

    return id;
}

int Sol_Prefab_Floor(World *world, vec3s pos)
{
    int id = Sol_Create_Ent(world);

    Sol_Xform_Add(world, id, pos);
    Sol_Model_Add(world, id, (ModelDesc){.id = SOL_MODEL_WORLD1});
    Sol_Body_Add(world, id, (BodyDesc){.shape = SHAPE3_MOD});

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

int Sol_Prefab_Ball(World *world, vec3s pos, vec3s vel, int ownerId, ShapeDesc desc)
{
    int id = Sol_Create_Ent(world);
    Sol_Shape_Add(world, id, desc);
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

    Sol_Contact_Add(world, id,
                    (ContactDesc){
                        .impacts = impact_list,
                        //.bounces = 0,
                    });
    Sol_Owner_Add(world, id, ownerId);

    return id;
}

int Sol_Prefab_Pawn(World *world, vec3s pos, vec2s dims, float scale, SolModelId modelid, MoveConfigId moveid)
{
    dims   = glms_vec2_scale(dims, scale);
    int id = Sol_Create_Ent(world);
    Sol_Xform_Add(world, id, pos);
    Sol_Body_Add(world, id,
                 (BodyDesc){
                     .height      = dims.y,
                     .radius      = dims.x,
                     .mass        = 1.0f,
                     .shape       = SHAPE3_CAP,
                     .restitution = 0.1f,
                     .group       = 1,
                 });

    Sol_Movement_Add(world, id, (MovementDesc){.configId = moveid});
    Sol_Model_Add(world, id, (ModelDesc){.id = modelid, .yoffset = -dims.y * 0.5f});
    Sol_Ability_Add(world, id, (AbilityDesc){0});

    return id;
}

int Sol_Prefab_Wizard(World *world, vec3s pos, float scale)
{
    vec2s dims = {.x = 0.5f, .y = 3.0f};

    dims   = glms_vec2_scale(dims, scale);
    int id = Sol_Create_Ent(world);
    Sol_Xform_Add(world, id, pos);
    Sol_Model_Add(world, id, (ModelDesc){.id = SOL_MODEL_WIZARD, .yoffset = -dims.y * 0.5f});
    Sol_Body_Add(world, id,
                 (BodyDesc){
                     .height      = dims.y,
                     .radius      = dims.x,
                     .mass        = 1.0f,
                     .shape       = SHAPE3_CAP,
                     .restitution = 0.1f,
                     .group       = 1,
                 });
    Sol_Movement_Add(world, id, (MovementDesc){.configId = MOVE_CONFIG_WIZARD});
    Sol_Ability_Add(world, id, (AbilityDesc){0});
    Sol_Interact_Add(world, id, (InteractDesc){0});
    Sol_Flags_Add(world, id, EFLAG_PICKUPABLE);
    Sol_Vital_Add(world, id, (VitalDesc){.maxHealth = 100, .maxMana = 100, .maxEnergy = 100});

    return id;
}

int Sol_Prefab_Button(World *world, vec3s pos, const char *text)
{
    float width  = 150.0f;
    float height = 50.0f;
    int   id     = Sol_Create_Ent(world);

    Sol_Xform_Add(world, id, pos);
    Sol_Body_Add(world, id, (BodyDesc){.height = height, .radius = width, .is2d = true});
    Sol_Interact_Add(world, id, (InteractDesc){0});

    Sol_Ui_Add(world, id,
               (UiDesc){
                   .kind            = UI_BUTTON,
                   .baseColor       = (vec4s){{255.0f, 0.0f, 0.0f, 255.0f}},
                   .borderColor     = (vec4s){{0.0f, 0.0f, 0.0f, 255.0f}},
                   .textColor       = (vec4s){{0.0f, 255.0f, 0.0f, 255.0f}},
                   .borderThickness = 2.0f,
                   .fontSize        = 16.0f,
                   .textWidth       = Sol_MeasureText(text, 16.0f, SOL_FONT_ICE),
                   .text            = text,
               });

    return id;
}
