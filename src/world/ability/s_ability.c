#include "ability_i.h"
#include "sol_core.h"
#include "world.h"
#include "sol_math.h"

#include "xform/s_xform.h"
#include "model/s_model.h"
#include "physx/s_body.h"
#include "controller/s_controller.h"
#include "buff/s_buff.h"
#include "replication/s_replication.h"
#include "vital/s_vital.h"
#include "render/render.h"

#define MAPPING_COUNT(maps) (sizeof(maps) / sizeof(AbilityMapping))

static void Ability_Step(World *world, double dt, double time);
static void Ability_Draw(World *world, double dt, double time);
static void Laser_Bounces(World *world, int id, vec3s pos, vec3s dir, int bounceCount);
static void Draw_Laser(vec3s startPos, vec3s endPos, double time, vec4s color, float width);
static void Draw_LaserImpact(vec3s pos, vec4s color, float scale, double time);
static void Draw_LaserStart(vec3s pos, vec4s color, float scale, double time);

void Sol_Ability_Init(World *world)
{
    world->abilities = calloc(MAX_ENTS, sizeof(CompAbility));
    WAddStep(world)  = Ability_Step;
    WAdd3d(world)    = Ability_Draw;

    Ability_Scripts_Init();
}

void Sol_Ability_Add(World *world, int id, AbilityDesc desc)
{
    CompAbility a = {0};
    memcpy(a.bindings, desc.bindings, sizeof(a.bindings));
    for (int i = 0; i < MAX_MAPPED_SKILLS; i++)
    {
        a.stateData[i].lastEntered = -FLT_MAX;
        a.stateData[i].lastExited  = -FLT_MAX;
        a.stateData[i].stage       = 0;
        a.stateData[i].elapsed     = 0.0f;
    }
    a.activeSlot = -1;

    world->masks[id] |= HAS_ABILITY;
    world->abilities[id] = a;
}

bool Sol_Ability_SetState(World *world, int id, AbilityState nextState, int slot, bool force)
{
    if (nextState > ABILITY_STATE_COUNT)
        return false;
    CompAbility     *ability  = &world->abilities[id];
    const StateFunc *prevfunc = &ability_state_func[ability->state];
    const StateFunc *nextfunc = &ability_state_func[nextState];

    if (!force)
    {
        if (!prevfunc->canExit || !prevfunc->canExit(world, id, nextState))
            return false;
        if (!nextfunc->canEnter || !nextfunc->canEnter(world, id, ability->state, nextState, slot))
            return false;
    }

    prevfunc->exit(world, id);
    ability->state                       = nextState;
    ability->activeSlot                  = slot;
    ability->stateData[slot].elapsed     = 0;
    ability->stateData[slot].accum       = 0;
    ability->stateData[slot].lastEntered = solState.gameTime;
    nextfunc->enter(world, id);
    return true;
}

AbilityState Sol_Ability_GetState(World *world, int id)
{
    return world->abilities[id].state;
}

void Sol_Ability_RequestBind(World *world, int id, u32 slot, u32 ability, u32 rarity, float bonusDamage, u32 bonusBuffs,
                             u32 bonusEffects)
{
    CompAbility *a = &world->abilities[id];
    if (slot >= MAX_MAPPED_SKILLS)
        return;
    SkillBinding *b = &a->bindings[slot];
    if (b->pendingState == ability && b->pendingRarity == rarity)
        return;
    b->pendingState        = ability;
    b->pendingRarity       = rarity;
    b->pendingBonusDamage  = bonusDamage;
    b->pendingBonusBuffs   = bonusBuffs;
    b->pendingBonusEffects = bonusEffects;

    b->dirtyApply = true;
    b->dirtySend  = true;
}

void Sol_Ability_Bind(World *world, int id, u32 slot, u32 ability, u32 rarity, float bonusDamage, u32 bonusBuffs,
                      u32 bonusEffects)
{
    CompAbility *a = &world->abilities[id];
    if (slot >= MAX_MAPPED_SKILLS)
        return;

    SkillBinding *b      = &a->bindings[slot];
    b->boundState        = ability;
    b->boundRarity       = rarity;
    b->boundBonusDamage  = bonusDamage;
    b->boundBonusBuffs   = bonusBuffs;
    b->boundBonusEffects = bonusEffects;

    b->pendingState        = ability;
    b->pendingRarity       = rarity;
    b->pendingBonusDamage  = bonusDamage;
    b->pendingBonusBuffs   = bonusBuffs;
    b->pendingBonusEffects = bonusEffects;

    AbilityData *data = &a->stateData[slot];
    data->cooldown    = ability_config[ability][rarity].cooldown;
    data->duration    = ability_config[ability][rarity].duration;
    data->damage      = ability_config[ability][rarity].damage + bonusDamage;
    data->buffs       = ability_config[ability][rarity].buffMask | bonusBuffs;
    data->effects     = ability_config[ability][rarity].effectMask | bonusEffects;
}

const char *Sol_Ability_GetNameString(u32 ability)
{
    return ability_config[ability]->name;
}

// PRIVATE

static void Ability_Step(World *world, double dt, double time)
{
    int required = HAS_ACTIVE | HAS_ABILITY;
    int count    = world->activeCount;
    for (int i = 0; i < count; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;
        if (Sol_Vital_GetDead(world, id))
        {
            Sol_Ability_SetState(world, id, ABILITY_STATE_IDLE, 0, true);
            continue;
        }
        if (Sol_Buff_HasBuff(world, id, BUFFKIND_STUN))
            continue;
        if (world->replications[id].auth == NETAUTH_REMOTE)
            continue;

        CompAbility *ability = &world->abilities[id];
        SolActions   actions = world->controllers[id].actionState;

        for (int m = 0; m < MAX_MAPPED_SKILLS; m++)
        {
            if (ability->bindings[m].dirtyApply)
            {
                Sol_Ability_Bind(world, id, m, ability->bindings[m].pendingState, ability->bindings[m].pendingRarity,
                                 ability->bindings[m].pendingBonusDamage, ability->bindings[m].pendingBonusBuffs,
                                 ability->bindings[m].pendingBonusEffects);
                ability->bindings[m].dirtyApply = false;
            }
        }
        for (int m = 0; m < MAX_MAPPED_SKILLS; m++)
        {
            SkillBinding *b = &ability->bindings[m];
            if (b->boundState == ABILITY_STATE_IDLE || b->actionBit == ACTION_NONE)
                continue;

            bool pressed               = (actions & b->actionBit) != 0;
            ability->stateData[m].held = pressed;

            if (pressed && ability->activeSlot != m)
            {
                Sol_Ability_SetState(world, id, b->boundState, m, false);
            }
        }

        if (ability->state != ABILITY_STATE_IDLE)
        {
            ability_state_func[ability->state].update(world, id, dt);
        }
    }
}

static void Ability_Draw(World *world, double dt, double time)
{
    int required = HAS_ABILITY;
    int count    = world->activeCount;
    for (int i = 0; i < count; i++)
    {
        int id = world->activeEntities[i];
        if ((world->masks[id] & required) != required)
            continue;

        CompXform      *xform      = &world->xforms[id];
        CompController *controller = &world->controllers[id];
        CompAbility    *ability    = &world->abilities[id];
        AbilityData    *data       = &ability->stateData[ability->activeSlot];

        switch (ability->state)
        {
        case ABILITY_STATE_FIREBALL: {

            if (data->stage > 0)
                break;
            float scale = data->charge * 2.0f + 0.5f;
            vec3s pos   = Sol_Model_GetBoneXform(world, id, "hand.L");
            pos         = vecAdd(pos, vecSca(Sol_Controller_GetAimdir(world, id), scale));
            pos         = vecAdd(pos, vecSca(WORLD_UP, scale));

            SphereSSBO *push = Sol_Render_GetNext_Fireball();
            push->pos        = (vec4s){pos.x, pos.y, pos.z, scale};
            push->color      = (vec4s){1, 0, 0, 0.8f};
        }
        break;
        case ABILITY_STATE_DASH:
            break;
        case ABILITY_STATE_CLAW:
            break;
        case ABILITY_STATE_SHIELD: {
            SphereSSBO *o = Sol_Render_GetNext_Sphere(true);
            o->pos        = (vec4s){xform->drawPos.x, xform->drawPos.y, xform->drawPos.z, 1.0f};
            o->color      = (vec4s){0.25f, 0.1f, 0.5f, 0.25f};
        }
        break;
        case ABILITY_STATE_LASER: {
            if (data->stage > 0)
            {
                const char *hand                     = ability->activeSlot == 1 ? "hand.R" : "hand.L";
                vec3s       startPos                 = Sol_Model_GetBoneXform(world, id, hand);
                float       widthScale               = Sol_Math_Lerp(0.2f, 2.5f, data->charge / 4.0f);
                data->as.laser.laserPointCountVisual = 0;
                Laser_Bounces(world, id, controller->aimpos, controller->aimdir, 0);
                for (int l = 0; l < data->as.laser.laserPointCountVisual; l += 2)
                {
                    vec3s posA = l == 0 ? startPos : data->as.laser.laserPointsVisual[l];
                    vec3s posB = data->as.laser.laserPointsVisual[l + 1];
                    Draw_LaserStart(posA, (vec4s){1.0f, 0.0f, 0.0f, 1.0f}, 0.5f * widthScale, time);
                    Draw_LaserStart(posA, (vec4s){1.0f, 1.0f, 1.0f, 1.0f}, 0.3f * widthScale, time);
                    Draw_LaserImpact(posB, (vec4s){1.0f, 0.0f, 0.0f, 1.0f}, 1.0f * widthScale, time);
                    Draw_LaserImpact(posB, (vec4s){1.0f, 1.0f, 1.0f, 1.0f}, 0.5f * widthScale, time);
                    Draw_Laser(posA, posB, time, (vec4s){0.0f, 0.0f, 0.0f, 1.0f}, 0.4f * widthScale);
                    Draw_Laser(posA, posB, time, (vec4s){0.5f, 0.0f, 0.0f, 1.0f}, 0.4f * widthScale);
                    Draw_Laser(posA, posB, time, (vec4s){1.0f, 0.0f, 0.0f, 1.0f}, 0.3f * widthScale);
                    Draw_Laser(posA, posB, time, (vec4s){1.0f, 1.0f, 1.0f, 1.0f}, 0.2f * widthScale);
                }
            }
        }
        break;
        }
    }
}

static void Laser_Bounces(World *world, int id, vec3s pos, vec3s dir, int bounceCount)
{
    if (bounceCount >= MAX_LASER_BOUNCES)
        return;
    CompAbility *ability                      = &world->abilities[id];
    AbilityData *data                         = &ability->stateData[ability->activeSlot];
    SolRay       ray                          = {.pos = pos, .dir = dir, .dist = LASER_LENGTH, .ignoreEnt = id};
    SolRayResult result                       = Sol_Raycast(world, ray);
    int          idx                          = data->as.laser.laserPointCountVisual;
    data->as.laser.laserPointsVisual[idx]     = pos;
    data->as.laser.laserPointsVisual[idx + 1] = result.pos;
    data->as.laser.laserPointCountVisual += 2;
    if (result.hit)
    {
        // Push the next ray origin outward by a tiny epsilon fraction along the hit normal
        vec3s biasedOrigin = glms_vec3_add(result.pos, glms_vec3_scale(result.norm, 0.001f));

        Laser_Bounces(world, id, biasedOrigin, Sol_BounceVec(dir, result.norm), bounceCount + 1);
    }
}

static const vec2s sprite_section[4] = {{0.0f, 0.5f}, {0.5f, 0.5f}, {0.5f, 0.0f}, {0.5f, 0.0f}};
static void        Draw_LaserImpact(vec3s pos, vec4s color, float scale, double time)
{
    u32       frame_index = ((u32)(time * 15.0)) % 4;
    vec2s     uv          = sprite_section[frame_index];
    QuadSSBO *ssbo        = Sol_Render_GetNext_Quad(QUADKIND_SPRITE_FRONT);
    ssbo->textureId       = SOL_TEXTURE_IMPACT;
    ssbo->color           = color;
    ssbo->pos             = (vec4s){pos.x, pos.y, pos.z, scale};
    ssbo->uv              = (vec4s){uv.x, uv.y, 0.5f, 0.5f};
}
static void Draw_LaserStart(vec3s pos, vec4s color, float scale, double time)
{
    u32       frame_index = ((u32)(time * 10.0)) % 4;
    vec2s     uv          = sprite_section[frame_index];
    QuadSSBO *ssbo        = Sol_Render_GetNext_Quad(QUADKIND_SPRITE);
    ssbo->textureId       = SOL_TEXTURE_IMPACT;
    ssbo->color           = color;
    ssbo->pos             = (vec4s){pos.x, pos.y, pos.z, scale};
    ssbo->uv              = (vec4s){uv.x, uv.y, 0.5f, 0.5f};
}

static void Draw_Laser(vec3s startPos, vec3s endPos, double time, vec4s color, float width)
{
    int   segments    = 4;
    float invSegs     = 1.0f / (float)(segments - 1);
    float uv_scroll_x = fmodf((float)time * -5.1f, 1.0f);
    float stretch     = 0.1f;

    for (int j = 0; j < segments - 1; j++)
    {
        float tA = (float)j * invSegs;
        float tB = (float)(j + 1) * invSegs;

        vec3s pA = glms_vec3_lerp(startPos, endPos, tA);
        vec3s pB = glms_vec3_lerp(startPos, endPos, tB);

        RibbonSegSSBO *seg = Sol_Render_GetNext_RibbonSeg(0);
        if (!seg)
            break;

        seg->posA   = (vec4s){pA.x, pA.y, pA.z, width};
        seg->posB   = (vec4s){pB.x, pB.y, pB.z, width};
        seg->colorA = color;
        seg->colorB = color;

        float distA    = glms_vec3_distance(startPos, pA);
        float distB    = glms_vec3_distance(startPos, pB);
        seg->uv        = (vec4s){distA * stretch, distB * stretch, uv_scroll_x, 0.0f};
        seg->textureId = SOL_TEXTURE_BEAM;
    }
}