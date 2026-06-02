#include "sol/sol.h"
#include "sol_core.h"

int Sol_Prefab_Factory(World *world, u32 id, u32 kind, PrefabDesc desc)
{
    NetAuth auth = desc.authority;
    if (auth == NETAUTH_NONE)
    {
        if (Net_IsActive())
        {
            if (Net_IsHost())
                auth = NETAUTH_AUTH;
            else
            {
                return 0;
                // auth = NETAUTH_LOCAL;
            }
        }
    }

    switch (kind)
    {
    case PREFABKIND_PLAYER:
        id = Sol_Prefab_Player(world, id, desc.pos, desc.scale);
        break;

    case PREFABKIND_WIZARD:
        id = Sol_Prefab_Wizard(world, id, desc.pos, desc.scale);
        break;

    case PREFABKIND_FIREBALL:
        id = Sol_Prefab_Fireball(world, id, desc.pos, desc.scale);
        break;
    }

    if (auth != NETAUTH_NONE)
        Sol_Replication_Add(world, id, auth, kind);

    return id;
}

int Sol_Prefab_Player(World *world, u32 id, vec3s pos, float scale)
{
    vec2s dims = {.x = 0.4f, .y = 1.65f};

    dims = glms_vec2_scale(dims, scale);
    id   = Sol_Create_Ent(world, id);
    if (id < 0)
        return -1;
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

    Sol_Movement_Add(world, id, MOVEMENTKIND_PLAYER);
    Sol_Vital_Add(world, id, VITALKIND_PLAYER);
    Sol_Ability_Add(world, id,
                    (AbilityDesc){.abilityMapping = {
                                      {ACTION_DASH, ABILITY_STATE_DASH},
                                      {ACTION_ABILITY1, ABILITY_STATE_FIREBALL},
                                      {ACTION_ABILITY2, ABILITY_STATE_SHIELD},
                                      {ACTION_ABILITY3, ABILITY_STATE_CLAW},
                                      {ACTION_ABILITY4, ABILITY_STATE_FIREBALLVOLLEY},
                                  }});
    Sol_Owner_Add(world, id, (OwnerDesc){.team = 1});
    return id;
}

int Sol_Prefab_Wizard(World *world, u32 id, vec3s pos, float scale)
{
    vec2s dims = {.x = 0.5f, .y = 3.0f};

    dims = glms_vec2_scale(dims, scale);
    id   = Sol_Create_Ent(world, id);
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
    Sol_Movement_Add(world, id, MOVEMENTKIND_WIZARD);
    Sol_Ability_Add(world, id,
                    (AbilityDesc){.abilityMapping = {
                                      {.actionBit = ACTION_ABILITY1, .targetState = ABILITY_STATE_FIREBALLVOLLEY},
                                  }});
    Sol_Interact_Add(world, id, (CompInteract){0});
    Sol_Flags_Add(world, id, EFLAG_PICKUPABLE);
    Sol_Vital_Add(world, id, VITALKIND_WIZARD);
    Sol_Owner_Add(world, id, (OwnerDesc){.team = 0});

    return id;
}

int Sol_Prefab_Fireball(World *world, u32 id, vec3s pos, float scale)
{
    float     radius = scale;
    ShapeDesc shape  = {.radius = radius, .color = {1, 0, 0, 1}, .kind = SHAPEKIND_FIREBALL};

    id = Sol_Create_Ent(world, id);
    if (id < 0)
        return -1;
    Sol_Shape_Add(world, id, shape);
    Sol_Xform_Add(world, id, pos);
    Sol_Xform_SetScale(world, id, (vec3s){scale, scale, scale});
    Sol_Body_Add(world, id,
                 (BodyDesc){
                     .radius         = shape.radius * 0.5f,
                     .shape          = SHAPE3_SPH,
                     .mass           = 1.0f * shape.radius,
                     .restitution    = 0.5f,
                     .group          = 0b10,
                     .ignoreFriendly = 1,
                 });
    Sol_Interact_Add(world, id, (CompInteract){0});
    Sol_Flags_Add(world, id, EFLAG_PICKUPABLE);
    Sol_Flags_Add(world, id, EFLAG_PROJECTILE);
    Sol_Contact_Add(world, id, CONTACTKIND_FIREBALL, scale);
    Sol_Contact_SetRadiusScale(world, id, scale);
    Sol_Replication_Add(world, id, 0, PREFABKIND_FIREBALL);
    Sol_Event_Add(world, (SolEvent){.kind        = EVENTKIND_FX,
                                    .as.fx.kind  = FXKIND_FIREBALL_SHOOT,
                                    .as.fx.pos   = pos,
                                    .as.fx.scale = scale,
                                    .as.fx.entA  = id});
    Sol_Emitter_Add(world, id, EMITTERKIND_FOUNTAIN_FIRE, scale);
    Sol_Emitter_Add(world, id, EMITTERKIND_FOUNTAIN_FOG, scale);
    Sol_Emitter_Add(world, id, EMITTERKIND_FOUNTAIN_SPARKS, scale);

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
    Sol_Interact_Add(world, id, (CompInteract){0});
    Sol_Flags_Add(world, id, EFLAG_PICKUPABLE);

    return id;
}

int Sol_Prefab_Clouds(World *world, vec3s pos)
{
    Sol_Emitter_SpawnEx(world, (Emitter){.pos       = pos,
                                         .inf       = 1,
                                         .burst     = 1000,
                                         .rate      = 0.2f,
                                         .rateBurst = 2,
                                         .particle  = (Particle){
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

int Sol_Prefab_Button(World *world, vec3s pos, const char *text)
{
    vec2s dims = {150.0f, 50.0f};
    int   id   = Sol_Create_Ent(world, 0);

    Sol_Xform_Add(world, id, pos);
    Sol_Body2d_Add(world, id, (CompBody2d){.dims = dims, .kind = BODY2DKIND_RECT});
    Sol_Interact_Add(world, id, (CompInteract){0});

    int bg = Sol_Create_Ent(world, 0);
    Sol_Xform_Add(world, bg, pos);
    Sol_View2d_Add(world, bg,
                   (CompView2d){
                       .kind       = VIEW2DKIND_RECT,
                       .color      = (vec4s){{1.0f, 0.0f, 0.0f, 1.0f}},
                       .hoverColor = (vec4s){{1.0f, 1.0f, 1.0f, 1.0f}},
                       .dims       = dims,
                   });
    Sol_Parent_Add(world, bg, (CompParent){.parentId = id});

    int border = Sol_Create_Ent(world, 0);
    Sol_Xform_Add(world, border, pos);
    Sol_View2d_Add(world, border,
                   (CompView2d){
                       .kind  = VIEW2DKIND_RECT,
                       .color = (vec4s){{0.0f, 0.0f, 0.0f, 1.0f}},
                       .dims  = dims,
                       .fill  = 3.0f,
                   });
    Sol_Parent_Add(world, border, (CompParent){.parentId = id});

    CompView2d textComp = {
        .kind  = VIEW2DKIND_TEXT,
        .color = (vec4s){{0.0f, 1.0f, 0.0f, 1.0f}},
        .dims  = {16.0f, 1.0f},
        .scale = 16.0f,
    };
    strncpy_s(textComp.text, sizeof(textComp.text), text, 64);
    int textId = Sol_Create_Ent(world, 0);
    Sol_Xform_Add(world, textId, pos);
    Sol_View2d_Add(world, textId, textComp);
    Sol_Parent_Add(world, textId,
                   (CompParent){.parentId = id, .localOffset = (vec3s){dims.x * 0.5f, dims.y * 0.5f, 0}});

    return id;
}