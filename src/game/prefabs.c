#include "sol/sol.h"
#include "sol_core.h"

int Sol_Prefab_Factory(World *world, u32 id, u32 kind, EntDesc desc)
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
    case ENTKIND_PLAYER:
        id = Sol_Prefab_Player(world, id, desc.pos, desc.scale);
        break;

    case ENTKIND_WIZARD:
        id = Sol_Prefab_Wizard(world, id, desc.pos, desc.scale);
        break;

    case ENTKIND_FIREBALL:
        id = Sol_Prefab_Fireball(world, id, desc.pos, desc.scale);
        break;
    case ENTKIND_BULLET:
        id = Sol_Prefab_Bullet(world, id, desc.pos, desc.scale);
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
    Sol_Xform_Teleport(world, id, pos);
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
                    (AbilityDesc){.bindings = {
                                      {ACTION_ABILITY1, 0},
                                      {ACTION_ABILITY2, 0},
                                      {ACTION_ABILITY3, 0},
                                      {ACTION_ABILITY4, 0},
                                      {ACTION_ABILITY5, 0},
                                      {ACTION_ABILITY6, 0},
                                      {ACTION_ABILITY7, 0},
                                      {ACTION_ABILITY8, 0},
                                      {ACTION_ABILITY9, 0},
                                      {ACTION_DASH, ABILITY_STATE_DASH},
                                  }});
    Sol_Owner_SetTeam(world, id, 1);
    return id;
}

int Sol_Prefab_Wizard(World *world, u32 id, vec3s pos, float scale)
{
    vec2s dims = {.x = 0.5f, .y = 3.0f};

    dims = glms_vec2_scale(dims, scale);
    id   = Sol_Create_Ent(world, id);
    if (id < 0)
        return -1;
    Sol_Xform_Teleport(world, id, pos);
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
                    (AbilityDesc){.bindings = {
                                      {ACTION_ABILITY1, ABILITY_STATE_FIREBALLVOLLEY},
                                  }});
    Sol_Interact_Set(world, id, (CompInteract){0});
    Sol_Flags_Add(world, id, EFLAG_PICKUPABLE);
    Sol_Vital_Add(world, id, VITALKIND_WIZARD);

    return id;
}

int Sol_Prefab_Fireball(World *world, u32 id, vec3s pos, float scale)
{
    float     radius = scale;
    ShapeDesc shape  = {.radius = radius, .color = {1, 0, 0, 1}, .kind = SHAPEKIND_FIREBALL};

    id = Sol_Create_Ent(world, id);
    Sol_Shape_Add(world, id, shape);
    Sol_Xform_Teleport(world, id, pos);
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
    Sol_Projectile_Add(world, id, PROJECTILEKIND_FIREBALL, scale);
    Sol_Flags_Add(world, id, EFLAG_PICKUPABLE);
    Sol_Flags_Add(world, id, EFLAG_PROJECTILE);
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

int Sol_Prefab_Bullet(World *world, u32 id, vec3s pos, float scale)
{
    float     radius = scale;
    ShapeDesc shape  = {.radius = radius, .color = {0, 1, 0, 1}, .kind = SHAPEKIND_SPHERE};
    u32       kind   = ENTKIND_BULLET;

    id = Sol_Create_Ent(world, id);
    Sol_Shape_Add(world, id, shape);
    Sol_Projectile_Add(world, id, PROJECTILEKIND_BULLET, 1.0f);
    Sol_Xform_Teleport(world, id, pos);
    Sol_Xform_SetScale(world, id, (vec3s){scale, scale, scale});
    Sol_Body_Add(world, id,
                 (BodyDesc){
                     .radius         = shape.radius,
                     .shape          = SHAPE3_SPH,
                     .mass           = 1.0f * shape.radius,
                     .restitution    = 0.5f,
                     .group          = 0b10,
                     .ignoreFriendly = 1,
                 });
    Sol_Flags_Add(world, id, EFLAG_PROJECTILE);
    Sol_Emitter_Add(world, id, EMITTERKIND_FOUNTAIN_SPARKS, scale);

    return id;
}

int Sol_Prefab_Floor(World *world, vec3s pos)
{
    int id = Sol_Create_Ent(world, 0);

    Sol_Xform_Teleport(world, id, pos);
    Sol_Model_Add(world, id, (ModelDesc){.id = SOL_MODEL_WORLD1});
    Sol_Body_Add(world, id, (BodyDesc){.shape = SHAPE3_MOD});

    return id;
}

int Sol_Prefab_Box(World *world, vec3s pos)
{

    int id = Sol_Create_Ent(world, 0);
    Sol_Xform_Teleport(world, id, pos);
    Sol_Model_Add(world, id, (ModelDesc){.id = SOL_MODEL_BOX});
    Sol_Body_Add(world, id, (BodyDesc){.mass = 0, .radius = 1.0f, .shape = SHAPE3_MOD, .group = 0b01});
    Sol_Interact_Set(world, id, (CompInteract){0});
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

int Sol_Prefab_Healthbar(World *world, vec3s pos, World *entWorld, u32 entId)
{
    vec2s dims = {250.0f, 30.0f};

    int bg = Sol_Create_Ent(world, 0);
    Sol_Xform_Add(world, bg, pos);
    Sol_View2d_Set(world, bg,
                   (CompView2d){
                       .count = 1,
                       .views[0] =
                           {
                               .kind       = VIEW2DKIND_RECT,
                               .dims       = {dims.x, dims.y},
                               .color      = {0, 0, 0, 1},
                               .hoverColor = {1, 1, 1, 0.5f},
                               .zindex     = 2,
                               .targetFill = 1.0f,
                               .fill       = 1.0f,
                           },
                   });
    Sol_Interact_Set(world, bg, (CompInteract){0});
    Sol_Body2d_Add(world, bg, BODY2DKIND_RECT, dims.x, dims.y, 0, 0);

    int bg2 = Sol_Create_Ent(world, 0);
    Sol_Xform_Add(world, bg2, pos);
    Sol_View2d_Set(world, bg2,
                   (CompView2d){
                       .count = 1,
                       .views[0] =
                           {
                               .kind       = VIEW2DKIND_RECT,
                               .dims       = {dims.x, dims.y},
                               .color      = {0.2, 0.2, 0.2, 1},
                               .hoverColor = {1, 1, 1, 0.5f},
                               .zindex     = 2,
                               .targetFill = 1.0f,
                               .fill       = 1.0f,
                               .textureID  = SOL_TEXTURE_HEALTH,
                           },
                   });
    Sol_Parent_Add(world, bg2, bg);

    int id = Sol_Create_Ent(world, 0);
    Sol_Xform_Teleport(world, id, pos);
    Sol_View2d_Set(world, id,
                   (CompView2d){.count    = 1,
                                .views[0] = {

                                    .kind       = VIEW2DKIND_RECT,
                                    .dims       = {dims.x, dims.y},
                                    .color      = {0, 1, 0, 1},
                                    .hoverColor = {1, 1, 0, 0.5f},
                                    .textureID  = SOL_TEXTURE_HEALTH,
                                    .targetFill = 1.0f,
                                    .fill       = 1.0f,
                                    .zindex     = 3,
                                }});
    Sol_Parent_Set(world, id, (CompParent){.active = true, .parentId = bg});
    Sol_World_SetOtherworld(world, id, entWorld, entId);

    int border = Sol_Create_Ent(world, 0);
    Sol_Xform_Teleport(world, border, pos);
    Sol_View2d_Set(world, border,
                   (CompView2d){
                       .count = 1,
                       .views[0] =
                           {
                               .kind       = VIEW2DKIND_RECT,
                               .dims       = {dims.x, dims.y},
                               .color      = {0, 0, 0, 1},
                               .hoverColor = {0, 0, 0, 0.5f},
                               .border     = 2.0f,
                               .fill       = 1.0f,
                               .targetFill = 1.0f,
                               .zindex     = 3,
                           },
                   });
    Sol_Parent_Set(world, border, (CompParent){.active = true, .parentId = bg});

    return id;
}

int Sol_Prefab_Button(World *world, vec3s pos, const char *text)
{
    vec2s dims = {150.0f, 50.0f};
    int   id   = Sol_Create_Ent(world, 0);

    Sol_Xform_Teleport(world, id, pos);
    Sol_Body2d_Add(world, id, BODY2DKIND_RECT, dims.x, dims.y, 0b1, 0b1);
    Sol_Interact_Set(world, id, (CompInteract){0});

    int bg = Sol_Create_Ent(world, 0);
    Sol_View2d_Set(world, bg,
                   (CompView2d){.count    = 1,
                                .views[0] = {
                                    .kind        = VIEW2DKIND_RECT,
                                    .color       = (vec4s){0.1f, 0.1f, 0.1f, 1.0f},
                                    .hoverColor  = (vec4s){1.0f, 1.0f, 1.0f, 1.0f},
                                    .toggleColor = (vec4s){0.0f, 0.5f, 0.5f, 1.0f},
                                    .fill        = 1.0f,
                                    .targetFill  = 1.0f,
                                    .dims        = {dims.x, dims.y},
                                }});
    int bg2 = Sol_Create_Ent(world, 0);
    Sol_View2d_Set(world, bg2,
                   (CompView2d){
                       .count = 1,
                       .views[0] =
                           {
                               .kind        = VIEW2DKIND_RECT,
                               .color       = (vec4s){0.5f, 0.1f, 0.1f, 1.0f},
                               .hoverColor  = (vec4s){1.0f, 1.0f, 1.0f, 1.0f},
                               .toggleColor = (vec4s){0.0f, 0.5f, 0.5f, 1.0f},
                               .dims        = {dims.x, dims.y},
                               .targetFill  = 1.0f,
                               .fill        = 1.0f,
                               .textureID   = SOL_TEXTURE_SWIRLFRAME,
                           },
                   });

    int border = Sol_Create_Ent(world, 0);
    Sol_View2d_Set(world, border,
                   (CompView2d){.count    = 1,
                                .views[0] = {
                                    .kind        = VIEW2DKIND_RECT,
                                    .color       = (vec4s){0.0f, 0.0f, 0.0f, 1.0f},
                                    .hoverColor  = (vec4s){0.0f, 0.0f, 0.0f, 1.0f},
                                    .clickColor  = (vec4s){0.0f, 0.0f, 0.0f, 1.0f},
                                    .toggleColor = (vec4s){0.0f, 0.0f, 0.0f, 1.0f},
                                    .dims        = {dims.x, dims.y},
                                    .border      = 3.0f,
                                    .fill        = 1.0f,
                                    .targetFill  = 1.0f,
                                }});
    Sol_Parent_Set(world, bg, (CompParent){.active = true, .parentId = id});
    Sol_Parent_Add(world, bg2, id);
    Sol_Parent_Set(world, border, (CompParent){.active = true, .parentId = id});

    CompView2d textComp = {
        .count = 1,
        .views[0] =
            {
                .kind       = VIEW2DKIND_TEXT,
                .color      = (vec4s){{0.0f, 1.0f, 0.0f, 1.0f}},
                .dims       = {16.0f},
                .fill       = 1.0f,
                .targetFill = 1.0f,
            },
    };
    strncpy_s(textComp.views[0].text, sizeof(textComp.views[0].text), text, 64);
    int textId = Sol_Create_Ent(world, 0);
    Sol_Xform_Teleport(world, textId, pos);
    Sol_View2d_Set(world, textId, textComp);
    Sol_Parent_Set(
        world, textId,
        (CompParent){.active = true, .parentId = id, .localOffset = (vec3s){dims.x * 0.5f, dims.y * 0.5f, 0}});

    return id;
}

int Sol_Prefab_AbilityCard(World *world, vec3s pos, AbilityState ability)
{
    vec2s dims = {60.0f, 70.0f};
    int   id   = Sol_Create_Ent(world, 0);
    Sol_Xform_Teleport(world, id, pos);
    Sol_Interact_Set(world, id, (CompInteract){0});
    CompBody2d *body = Sol_Body2d_Add(world, id, BODY2DKIND_RECT, dims.x, dims.y, 1, 1);
    Sol_Parent_Set(world, id, (CompParent){.active = false});
    Sol_Body2d_SetOverlapMask(world, id, 0b10, 0b01);
    SolView2d *image         = Sol_View2d_Add(world, id, VIEW2DKIND_RECT, (vec4s){1, 1, 1, 1}, dims.x, dims.y);
    image->textureUV         = (vec2s){1, 0.816};
    image->hoverColor        = (vec4s){0.5f, 0.5f, 0.5f, 1.0f};
    world->view2d[id].zindex = 1;

    Sol_Item_AddAbility(world, id, ability);
    switch (ability)
    {
    case ABILITY_STATE_FIREBALL:
        image->textureID = SOL_TEXTURE_FIREBALL_CARD;
        break;
    case ABILITY_STATE_PISTOL:
        image->textureID = SOL_TEXTURE_PISTOL_CARD;
        break;
    case ABILITY_STATE_SHIELD:
        image->textureID = SOL_TEXTURE_CRYSTAL_CARD;
        break;
    case ABILITY_STATE_SPINSLASH:
        image->textureID = SOL_TEXTURE_BLADE_CARD;
        break;
    }

    return id;
}

int Sol_Prefab_AbilitySlot(World *world, vec3s pos, u32 slot)
{
    vec2s dims = {70.0f, 70.0f};
    int   id   = Sol_Create_Ent(world, 0);
    Sol_Xform_Add(world, id, pos);
    Sol_Item_AddAbilitySlot(world, id, slot);
    CompBody2d *body   = Sol_Body2d_Add(world, id, BODY2DKIND_RECT, dims.x, dims.y, 0, 0);
    body->overlapGroup = 0b01;
    body->overlapMask  = 0b10;

    SolView2d *view = Sol_View2d_Add(world, id, VIEW2DKIND_RECT, (vec4s){0.5f, 0.5f, 0.5f, 1.0f}, dims.x, dims.y);
    view->zindex     = 0;

    SolView2d *border = Sol_View2d_Add(world, id, VIEW2DKIND_RECT, (vec4s){0.0f, 0.0f, 0.0f, 1.0f}, dims.x, dims.y);
    border->hoverColor = (vec4s){0.2f, 0.2f, 0.2f, 1.0f};
    border->textureID  = SOL_TEXTURE_SWIRLFRAME;

    SolView2d *border2 = Sol_View2d_Add(world, id, VIEW2DKIND_RECT, (vec4s){0, 0, 0, 1.0f}, dims.x, dims.y);
    border2->border = 4.0f;
    SolView2d *cooldown = Sol_View2d_Add(world, id, VIEW2DKIND_RECT, (vec4s){1.0f, 0, 0, 0.7f}, dims.x, dims.y);
    cooldown->zindex    = 2;
    cooldown->flags     = (1 << 1 | 1 << 2);
    cooldown->textureID = SOL_TEXTURE_SPIKEFRAMEFILLED;
    SolView2d *text = Sol_View2d_Add(world, id, VIEW2DKIND_TEXT, (vec4s){1.0f, 1.0f, 1.0f, 1.0f}, 20.0f, 0);
    snprintf(text->text, sizeof(text->text), "%d", slot + 1);
    text->offset.x = dims.x * 0.5f;
    text->offset.y = dims.y * 0.5f;

    return id;
}