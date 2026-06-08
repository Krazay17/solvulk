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
    Sol_Model_Add(world, id, MODELKIND_DUDE, dims.y);
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
                                      {ACTION_DASH, 0},
                                  }});
    Sol_Owner_SetTeam(world, id, 1);
    return id;
}

int Sol_Prefab_Wizard(World *world, u32 id, vec3s pos, float scale)
{
    vec2s dims = {.x = 0.5f, .y = 3.0f};
    glms_vec2_scale(dims, scale);

    dims = glms_vec2_scale(dims, scale);
    id   = Sol_Create_Ent(world, id);
    if (id < 0)
        return -1;
    Sol_Xform_Add(world, id, pos);
    Sol_Xform_SetScale(world, id, (vec3s){scale, scale, scale});
    Sol_Model_Add(world, id, MODELKIND_WIZARD, dims.y);
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
    Sol_Model_Add(world, id, SOL_MODEL_WORLD1, 1.0f);
    Sol_Body_Add(world, id, (BodyDesc){.shape = SHAPE3_MOD});

    return id;
}

int Sol_Prefab_Box(World *world, vec3s pos)
{

    int id = Sol_Create_Ent(world, 0);
    Sol_Xform_Teleport(world, id, pos);
    Sol_Model_Add(world, id, SOL_MODEL_BOX, 1.0f);
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

    int id = Sol_Create_Ent(world, 0);
    Sol_Xform_Add(world, id, pos);
    Sol_Body2d_Add(world, id, BODY2DKIND_RECT, dims.x, dims.y, 0, 0);
    Sol_Interact_Add(world, id);
    Sol_World_SetTracker(world, id, entWorld, entId);
    Sol_Flags_Add(world, id, EFLAG_HEALTHBAR);

    SolView2d *bg  = Sol_View2d_Add(world, id, VIEW2DKIND_RECT, (vec4s){0, 0, 0, 1}, dims.x, dims.y);
    bg->zindex     = 2;
    bg->hoverColor = (vec4s){1, 1, 1, 0.5f};

    SolView2d *bg2 = Sol_View2d_Add(world, id, VIEW2DKIND_RECT, (vec4s){0.2, 0.2, 0.2, 1}, dims.x, dims.y);
    bg2->zindex    = 2;
    bg2->textureID = SOL_TEXTURE_HEALTH;

    SolView2d *bar  = Sol_View2d_Add(world, id, VIEW2DKIND_RECT, (vec4s){1, 0, 0, 1}, dims.x, dims.y);
    bar->zindex     = 2;
    bar->fillSpeed  = 4.0f;
    bar->hoverColor = (vec4s){1, 1, 0, 0.5f};
    bar->textureID  = SOL_TEXTURE_HEALTH;

    SolView2d *bar2  = Sol_View2d_Add(world, id, VIEW2DKIND_RECT, (vec4s){0, 1, 0, 1}, dims.x, dims.y);
    bar2->zindex     = 2;
    bar2->hoverColor = (vec4s){1, 1, 0, 0.5f};
    bar2->textureID  = SOL_TEXTURE_HEALTH;

    SolView2d *border = Sol_View2d_Add(world, id, VIEW2DKIND_RECT, (vec4s){0, 0, 0, 1}, dims.x, dims.y);
    border->zindex    = 2;
    border->border    = 2.0f;

    return id;
}

int Sol_Prefab_Button(World *world, vec3s pos, const char *text)
{
    vec2s dims = {150.0f, 50.0f};
    int   id   = Sol_Create_Ent(world, 0);
    Sol_Xform_Teleport(world, id, pos);
    CompBody2d *body = Sol_Body2d_Add(world, id, BODY2DKIND_RECT, dims.x, dims.y, 0b1, 0b1);

    SolView2d *bg   = Sol_View2d_Add(world, id, VIEW2DKIND_RECT, (vec4s){0.1f, 0.1f, 0.1f, 1.0f}, dims.x, dims.y);
    bg->hoverColor  = (vec4s){1.0f, 1.0f, 1.0f, 1.0f};
    bg->toggleColor = (vec4s){0.0f, 0.5f, 0.5f, 1.0f};

    SolView2d *bg2   = Sol_View2d_Add(world, id, VIEW2DKIND_RECT, (vec4s){0.5f, 0.1f, 0.1f, 1.0f}, dims.x, dims.y);
    bg2->hoverColor  = (vec4s){1.0f, 1.0f, 1.0f, 1.0f};
    bg2->toggleColor = (vec4s){0.0f, 0.5f, 0.5f, 1.0f};
    bg2->textureID   = SOL_TEXTURE_SWIRLFRAME;

    SolView2d *border = Sol_View2d_Add(world, id, VIEW2DKIND_RECT, (vec4s){0.0f, 0.0f, 0.0f, 1.0f}, dims.x, dims.y);
    border->border    = 3.0f;

    SolView2d *textView = Sol_View2d_Add(world, id, VIEW2DKIND_TEXT, (vec4s){0.0f, 1.0f, 0.0f, 1.0f}, 16.0f, 0);
    strncpy_s(textView->text, sizeof(textView->text), text, 64);
    textView->offset = (vec4s){dims.x * 0.5f, dims.y * 0.5f};

    return id;
}

int Sol_Prefab_AbilityCard(World *world, vec3s pos, AbilityState ability)
{
    vec2s dims = {62.0f, 62.0f};

    int id = Sol_Create_Ent(world, 0);
    Sol_Xform_Teleport(world, id, pos);
    Sol_Interact_Add(world, id);
    CompBody2d *body = Sol_Body2d_Add(world, id, BODY2DKIND_RECT, dims.x, dims.y, 1, 1);
    body->zindex     = 1;
    Sol_Body2d_SetOverlapMask(world, id, 0b10, 0b01);
    Sol_Parent_SetActive(world, id, false);
    Sol_Item_AddAbility(world, id, ability);
    CompTooltip *tooltip = Sol_Tooltip_Add(world, id, TOOLTIPKIND_CARD);

    SolView2d *image         = Sol_View2d_Add(world, id, VIEW2DKIND_RECT, (vec4s){1, 1, 1, 1}, dims.x, dims.y);
    image->textureUV         = (vec2s){1, 0.816};
    image->hoverColor        = (vec4s){0.5f, 0.5f, 0.5f, 1.0f};
    image->zindex            = 2;
    world->view2d[id].zindex = 1;

    switch (ability)
    {
    case ABILITY_STATE_FIREBALL:
        image->textureID = SOL_TEXTURE_FIREBALL_CARD;
        snprintf(tooltip->header, sizeof(tooltip->header), "Fireball");
        break;
    case ABILITY_STATE_PISTOL:
        image->textureID = SOL_TEXTURE_PISTOL_CARD;
        snprintf(tooltip->header, sizeof(tooltip->header), "Pistol");
        break;
    case ABILITY_STATE_SHIELD:
        image->textureID = SOL_TEXTURE_CRYSTAL_CARD;
        snprintf(tooltip->header, sizeof(tooltip->header), "Shield");
        break;
    case ABILITY_STATE_SPINSLASH:
        image->textureID = SOL_TEXTURE_BLADE_CARD;
        snprintf(tooltip->header, sizeof(tooltip->header), "SpinSlash");
        break;
    case ABILITY_STATE_DASH:
        image->textureID = SOL_TEXTURE_DASH_CARD;
        snprintf(tooltip->header, sizeof(tooltip->header), "Dash");
        break;
    }
    SolView2d *border  = Sol_View2d_Add(world, id, VIEW2DKIND_RECT, (vec4s){0, 0, 0, 1}, dims.x, dims.y);
    border->zindex     = 2;
    border->textureID  = SOL_TEXTURE_BORDER;
    border->hoverColor = (vec4s){1.0f, 1.0f, 1.0f, 1.0f};

    return id;
}

int Sol_Prefab_AbilitySlot(World *world, vec3s pos, u32 slot, char *label)
{
    vec2s dims = {70.0f, 70.0f};
    int   id   = Sol_Create_Ent(world, 0);
    Sol_Xform_Add(world, id, pos);
    Sol_Item_AddAbilitySlot(world, id, slot);
    CompBody2d *body   = Sol_Body2d_Add(world, id, BODY2DKIND_RECT, dims.x, dims.y, 0, 0);
    body->overlapGroup = 0b01;
    body->overlapMask  = 0b10;

    // 0
    SolView2d *view = Sol_View2d_Add(world, id, VIEW2DKIND_RECT, (vec4s){0.5f, 0.5f, 0.5f, 1.0f}, dims.x, dims.y);
    view->zindex    = 0;

    // 1
    SolView2d *border  = Sol_View2d_Add(world, id, VIEW2DKIND_RECT, (vec4s){0.0f, 0.0f, 0.0f, 1.0f}, dims.x, dims.y);
    border->hoverColor = (vec4s){0.2f, 0.2f, 0.2f, 1.0f};
    border->textureID  = SOL_TEXTURE_SWIRLFRAME;
    border->zindex     = 1;

    // 2
    SolView2d *border2  = Sol_View2d_Add(world, id, VIEW2DKIND_RECT, (vec4s){0, 0, 0, 1.0f}, dims.x, dims.y);
    border2->border     = 4.0f;
    border2->zindex     = 4;
    border2->hoverColor = (vec4s){1.0f, 1.0f, 1.0f, 1.0f};

    // 3
    SolView2d *press = Sol_View2d_Add(world, id, VIEW2DKIND_RECT, (vec4s){0.5f, 0.5f, 0.5f, 0.0f}, dims.x, dims.y);
    press->textureID = SOL_TEXTURE_CLOUD1;
    press->zindex    = 3;

    // 4
    SolView2d *text = Sol_View2d_Add(world, id, VIEW2DKIND_TEXT, (vec4s){1.0f, 1.0f, 1.0f, 1.0f}, 20.0f, 0);
    strcpy(text->text, label);
    text->offset.x = dims.x * 0.5f;
    text->offset.y = dims.y * 0.5f;
    text->zindex   = 3;

    // 5
    SolView2d *active = Sol_View2d_Add(world, id, VIEW2DKIND_RECT, (vec4s){1.0f, 1.0f, 1.0f, 0.0f}, dims.x, dims.y);
    active->textureID = SOL_TEXTURE_SPIKEFRAMEFILLED;
    active->flags     = (1 << 1);
    active->zindex    = 3;

    // 6
    SolView2d *cooldown = Sol_View2d_Add(world, id, VIEW2DKIND_RECT, (vec4s){1.0f, 0, 0, 0.95f}, dims.x, dims.y - 2.0f);
    cooldown->flags     = (1 << 1 | 1 << 2);
    cooldown->textureID = SOL_TEXTURE_CLOUD2;
    cooldown->zindex    = 3;
    cooldown->offset.y  = -0.5f;
    // 7

    SolView2d *cdFlash  = Sol_View2d_Add(world, id, VIEW2DKIND_RECT, (vec4s){1.0f, 1.0f, 1.0f, 0.0f}, dims.x, dims.y);
    cdFlash->clickColor = (vec4s){1.0f, 1.0f, 1.0f, 1.0f};
    cdFlash->textureID  = SOL_TEXTURE_SHOCKPARTICLE;
    cdFlash->zindex     = 3;

    return id;
}