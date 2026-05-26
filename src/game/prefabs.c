#include "sol/sol.h"

// static BuffDesc buffs[] = {
//     {.kind = BUFFKIND_KNOCKBACK, .duration = 0.5f},
//     {.kind = BUFFKIND_FIRE, .duration = 5.0f, .freq = 0.2f},
// };

static ImpactList fireball_impacts = {
    .impacts[0] =
        {
            .kind = IMPACT_DIRECT,
            .hit =
                {
                    .damage = 15,
                    .kind   = HITKIND_FIRE,
                    // .buffs =
                    //     {
                    //         {.kind = BUFFKIND_KNOCKBACK, .duration = 0.5f},
                    //     },
                    // .power     = 5.0f,
                    // .buffcount = 1,
                },
        },
    .impacts[1] =
        {
            .kind = IMPACT_AOE,
            .hit =
                {
                    .damage = 10,
                    .kind   = HITKIND_FIRE,
                    .buffs =
                        {
                            {.kind = BUFFKIND_FIRE, .duration = 3.0f, .freq = 0.5f, .addKind = BUFFADD_SET_DURATION},
                        },
                    .power     = 10.0f,
                    .buffcount = 1,
                },
            .radius = 3.0f,
        },

    .impactCount = 2,
};

int Sol_Prefab_Player(World *world, u32 id, vec3s pos, float scale)
{
    vec2s dims = {.x = 0.35f, .y = 2.0f};

    dims   = glms_vec2_scale(dims, scale);
    id = Sol_Create_Ent(world, 0);
    Sol_Xform_Add(world, id, pos);
    Sol_Model_Add(world, id, (ModelDesc){.id = SOL_MODEL_DUDE, .yoffset = -dims.y * 0.5f, .yawOffset = GLM_PI_2f});
    Sol_Body_Add(world, id,
                 (BodyDesc){
                     .height      = dims.y,
                     .radius      = dims.x,
                     .mass        = 1.0f,
                     .shape       = SHAPE3_CAP,
                     .restitution = 0.01f,
                     .group       = 1,
                 });

    Sol_Movement_Add(world, id, (MovementDesc){.configId = MOVE_CONFIG_PLAYER});
    Sol_Vital_Add(world, id, (VitalDesc){.maxHealth = 100, .maxMana = 100, .maxEnergy = 100});
    Sol_Ability_Add(world, id,
                    (AbilityDesc){.abilityMapping = {
                                      {ACTION_DASH, ABILITY_STATE_DASH},
                                      {ACTION_ABILITY1, ABILITY_STATE_FIREBALL},
                                      {ACTION_ABILITY2, ABILITY_STATE_SHIELD},
                                      {ACTION_ABILITY3, ABILITY_STATE_CLAW},
                                      {ACTION_ABILITY4, ABILITY_STATE_FIREBALLVOLLEY},
                                  }});

    return id;
}

int Sol_Prefab_Floor(World *world, vec3s pos)
{
    int id = Sol_Create_Ent(world, 0);

    Sol_Xform_Add(world, id, pos);
    Sol_Model_Add(world, id, (ModelDesc){.id = SOL_MODEL_WORLD1});
    Sol_Body_Add(world, id, (BodyDesc){.shape = SHAPE3_MOD});

    return id;
}

int Sol_Prefab_Box(World *world, vec3s pos)
{

    int id = Sol_Create_Ent(world, 0);
    Sol_Xform_Add(world, id, pos);
    Sol_Model_Add(world, id, (ModelDesc){.id = SOL_MODEL_BOX});
    Sol_Body_Add(world, id, (BodyDesc){.mass = 0, .radius = 1.0f, .shape = SHAPE3_MOD, .group = 0b01});
    Sol_Interact_Add(world, id, (InteractDesc){0});
    Sol_Flags_Add(world, id, EFLAG_PICKUPABLE);

    return id;
}

int Sol_Prefab_Clouds(World *world, vec3s pos)
{
    Sol_Emitter_Add(world, (Emitter){.pos      = pos,
                                     .burst    = 1000,
                                     .inf      = 1,
                                     .rate     = 0.1f,
                                     .particle = (Particle){
                                         .randScale = 1,
                                         .ttl       = 120.0f,
                                         .color     = (vec4s){.r = 1, .g = 1, .b = 1, .a = 0.2f},
                                         .scale     = 40.0f,
                                         .kind      = PARTICLE_CLOUD,
                                         .rotspeed  = Sol_Math_RandRange(-.1f, .1f),
                                         .speed     = 1.0f,
                                         .offset    = 100.0f,
                                         .scalein   = 0.05f,
                                         .scaleout  = 0.05f,
                                     }});
    return 0;
}

int Sol_Prefab_Ball(World *world, vec3s pos, vec3s vel, int ownerId, ShapeDesc shape, ContactDesc contact)
{
    int id = Sol_Create_Ent(world, 0);
    Sol_Shape_Add(world, id, shape);
    Sol_Xform_Add(world, id, pos);
    Sol_Body_Add(world, id,
                 (BodyDesc){
                     .radius         = shape.radius,
                     .shape          = SHAPE3_SPH,
                     .mass           = 1.0f * shape.radius,
                     .restitution    = 0.5f,
                     .vel            = vel,
                     .group          = 0b10,
                     .ignoreFriendly = 1,
                 });
    Sol_Interact_Add(world, id, (InteractDesc){0});
    Sol_Flags_Add(world, id, EFLAG_PICKUPABLE);

    Sol_Contact_Add(world, id, contact);
    Sol_Owner_SetOwner(world, id, ownerId);
    Sol_Flags_Add(world, id, EFLAG_PROJECTILE);

    return id;
}

int Sol_Prefab_Fireball(World *world, int ownerId, vec3s pos, vec3s vel, float radius, u32 damage)
{
    ShapeDesc shape = {.radius = radius, .color = {1, 0, 0, 1}, .kind = SHAPEKIND_FIREBALL};

    int id = Sol_Create_Ent(world, 0);
    Sol_Shape_Add(world, id, shape);
    Sol_Xform_Add(world, id, pos);
    Sol_Body_Add(world, id,
                 (BodyDesc){
                     .radius         = shape.radius * 0.5f,
                     .shape          = SHAPE3_SPH,
                     .mass           = 1.0f * shape.radius,
                     .restitution    = 0.5f,
                     .vel            = vel,
                     .group          = 0b10,
                     .ignoreFriendly = 1,
                 });
    Sol_Interact_Add(world, id, (InteractDesc){0});
    Sol_Flags_Add(world, id, EFLAG_PICKUPABLE);

    Sol_Contact_Add(world, id,
                    (ContactDesc){.impacts = {
                                      .impactCount = 2,
                                      .impacts[0] =
                                          {
                                              .kind   = IMPACT_DIRECT,
                                              .radius = radius,
                                              .hit =
                                                  {
                                                      .kind   = HITKIND_FIRE,
                                                      .damage = damage,
                                                  },
                                          },
                                      .impacts[1] =
                                          {
                                              .kind   = IMPACT_AOE,
                                              .radius = radius * 2.0f,
                                              .hit =
                                                  {
                                                      .damage    = damage / 2,
                                                      .kind      = HITKIND_FIRE,
                                                      .buffs     = {{
                                                          .kind     = BUFFKIND_FIRE,
                                                          .addKind  = BUFFADD_SET_DURATION,
                                                          .duration = 3.0f,
                                                          .freq     = 0.5f,
                                                      }},
                                                      .power     = 10.0f,
                                                      .buffcount = 1,
                                                  },
                                          },
                                  }});
    Sol_Owner_SetOwner(world, id, ownerId);
    Sol_Flags_Add(world, id, EFLAG_PROJECTILE);

    return id;
}

int Sol_Prefab_Pawn(World *world, vec3s pos, vec2s dims, float scale, SolModelId modelid, MoveConfigId moveid)
{
    dims   = glms_vec2_scale(dims, scale);
    int id = Sol_Create_Ent(world, 0);
    Sol_Xform_Add(world, id, pos);
    Sol_Body_Add(world, id,
                 (BodyDesc){
                     .height      = dims.y,
                     .radius      = dims.x,
                     .mass        = 1.0f,
                     .shape       = SHAPE3_CAP,
                     .restitution = 0.01f,
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
    int id = Sol_Create_Ent(world, 0);
    if (id < 0)
        return -1;
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
    Sol_Ability_Add(world, id,
                    (AbilityDesc){.abilityMapping = {
                                      {.actionBit = ACTION_ABILITY1, .targetState = ABILITY_STATE_FIREBALLVOLLEY},
                                  }});
    Sol_Interact_Add(world, id, (InteractDesc){0});
    Sol_Flags_Add(world, id, EFLAG_PICKUPABLE);
    Sol_Vital_Add(world, id, (VitalDesc){.maxHealth = 100, .maxMana = 100, .maxEnergy = 100});
    Sol_Owner_Add(world, id, (OwnerDesc){.team = 1});

    return id;
}

int Sol_Prefab_Button(World *world, vec3s pos, const char *text)
{
    float width  = 150.0f;
    float height = 50.0f;
    int   id     = Sol_Create_Ent(world, 0);

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
