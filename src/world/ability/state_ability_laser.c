#include "ability_i.h"
#include "sol_core.h"
#include "world.h"
#include "sol_math.h"
#include "audio.h"
#include "audio/s_audio.h"
#include "xform/s_xform.h"
#include "controller/s_controller.h"
#include "combat/s_combat.h"
#include "model/s_model.h"
#include "event/s_event.h"
#include "physx/s_body.h"
#include "movement/s_movement.h"
#include "render/render.h"

#define HITINTERVAL 0.1f
#define HITDELAY 0.25f
#define MAX_DURATION 3.5f
#define MAX_CHARGE 2.0f
#define MIN_CHARGE 0.2f
#define LASER_LENGTH 25.0f
#define MAX_LASER_BOUNCES 8

static void Laser_Bounces_Visual(World *world, int id, vec3s pos, vec3s dir, int bounceCount);
static void Draw_Laser(vec3s startPos, vec3s endPos, double time, vec4s color, float width);
static void Draw_LaserImpact(vec3s pos, vec4s color, float scale, double time);
static void Draw_LaserStart(vec3s pos, vec4s color, float scale, double time);

static void Laser_Bounces(World *world, int id, vec3s pos, vec3s dir, int bounceCount)
{
    if (bounceCount >= MAX_LASER_BOUNCES)
        return;
    CompAbility *ability                = &world->abilities[id];
    AbilityData *data                   = &ability->stateData[ability->activeSlot];
    SolRay       ray                    = {.pos = pos, .dir = dir, .dist = LASER_LENGTH, .ignoreEnt = id};
    SolRayResult result                 = Sol_Raycast(world, ray);
    int          idx                    = data->as.laser.laserPointCount;
    data->as.laser.laserPoints[idx]     = pos;
    data->as.laser.laserPoints[idx + 1] = result.pos;
    data->as.laser.laserPointCount += 2;
    if (result.hit)
    {
        // Push the next ray origin outward by a tiny epsilon fraction along the hit normal
        vec3s biasedOrigin = glms_vec3_add(result.pos, glms_vec3_scale(result.norm, 0.001f));

        Laser_Bounces(world, id, biasedOrigin, Sol_BounceVec(dir, result.norm), bounceCount + 1);
    }
}

static bool Leave_State(World *world, int id, CompAbility *ability)
{
    if (!ability->stateData[ability->activeSlot].held)
        if (Sol_Ability_SetState(world, id, ABILITY_STATE_IDLE, 0, true))
            return true;
    return false;
}

void Laser_State_Update(World *world, int id, float dt)
{
    CompAbility *ability = &world->abilities[id];
    if (Leave_State(world, id, ability))
        return;
    AbilityData *data = &ability->stateData[ability->activeSlot];
    data->elapsed += dt;

    CompController *controller = &world->controllers[id];

    if (Sol_Physx_GetVel(world, id).y < 0.0f && (Sol_Movement_GetState(world, id) == MOVE_FALL))
        Sol_Physx_SetVelY(world, id, fmax(Sol_Physx_GetVel(world, id).y, 0.0f));

    if (data->elapsed < HITDELAY)
        return;

    if (data->stage < 1)
    {
        data->stage   = 1;
        AnimDesc desc = {.anim     = ability->activeSlot == 1 ? ANIM_CHANNEL_RIGHT : ANIM_CHANNEL_LEFT,
                         .layerId  = ANIM_LAYER_UPPER,
                         .speed    = 1.0f,
                         .playKind = ANIMPLAYKIND_LOOP,
                         .blendIn  = 0.1f};
        Sol_Model_PlayAnim(world, id, desc);
    }

    if (data->stage == 1)
    {
        data->charge += dt * 8.0f;
        if (data->charge >= MAX_CHARGE)
            data->stage = 2;
    }
    else
        data->charge -= dt * 1.0f;

    data->charge = fminf(MAX_CHARGE, fmaxf(MIN_CHARGE, data->charge));

    data->accum += dt;
    if (data->accum < HITINTERVAL)
        return;
    data->accum = 0;

    CompCombat *combat = &world->combats[id];
    Sol_Combat_ClearHits(world, id);

    float finalCharge = data->charge * HITINTERVAL;
    float finalDamage = data->damage * finalCharge;

    data->as.laser.laserPointCount = 0;
    Laser_Bounces(world, id, controller->aimpos, controller->aimdir, 0);
    for (int b = 0; b < data->as.laser.laserPointCount; b += 2)
    {
        vec3s  posA      = data->as.laser.laserPoints[b];
        vec3s  posB      = data->as.laser.laserPoints[b + 1];
        vec3s  dir       = vecNorm(vecSub(posB, posA));
        SolRay rayDamage = {
            .dir       = dir,
            .dist      = glms_vec3_distance(posB, posA),
            .ignoreEnt = id,
            .pos       = posA,
            .mask      = 0b1,
        };

        SolRayResult results[256];
        int          hits =
            Sol_SphereCast(world, rayDamage, Sol_Math_Lerp(0.1f, 1.25f, data->charge / MAX_CHARGE), results, 256);
        for (int i = 0; i < hits; i++)
        {
            SolRayResult result = results[i];

            if (combat->hitEnts[result.entId])
                continue;
            combat->hitEnts[result.entId] = true;

            SolHit hit = {
                .damage     = finalDamage,
                .power      = finalCharge,
                .buffMask   = data->buffs,
                .effectMask = data->effects,
                .entA       = id,
                .entB       = result.entId,
                .pos        = result.pos,
                .vel        = dir,
            };
            Sol_Combat_ApplyHit(world, result.entId, hit);
        }
    }
}

void Laser_State_Enter(World *world, int id)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ability->activeSlot];
    CompCombat  *combat  = &world->combats[id];
    data->accum          = HITINTERVAL;
    data->charge         = 0;
    data->stage          = 0;
    Sol_Combat_ClearHits(world, id);

    Sol_World_Audio_Add(world, id, SOL_AUDIO_LASER, 0.3f, 0);

    float animRate           = 1.2f;
    combat->baseAnimRate     = animRate;
    combat->hitPause         = 0;
    combat->hitPauseDiminish = 0;

    AnimDesc desc = {.anim     = ability->activeSlot == 1 ? ANIM_ATTACK_RIGHT : ANIM_ATTACK_LEFT,
                     .layerId  = ANIM_LAYER_UPPER,
                     .speed    = animRate,
                     .playKind = ANIMPLAYKIND_LOOP,
                     .blendIn  = 0.05f};

    Sol_Model_PlayAnim(world, id, desc);
}

void Laser_State_Exit(World *world, int id)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ability->activeSlot];
    data->lastExited     = solState.gameTime;
    Sol_Model_PlayAnim(world, id, (AnimDesc){.layerId = ANIM_LAYER_UPPER});
    Sol_World_Audio_Remove(world, id, 0);
}

bool Laser_State_CanExit(World *world, int id, u32 next)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[ability->activeSlot];
    return true; // data->elapsed >= data->duration * 0.8f;
}

bool Laser_State_CanEnter(World *world, int id, u32 last, u32 next, int slot)
{
    CompAbility *ability = &world->abilities[id];
    AbilityData *data    = &ability->stateData[slot];
    return slot != ability->activeSlot && !(data->lastExited + data->cooldown > solState.gameTime);
}

void Laser_State_Draw(World *world, int id, double dt, double time)
{
    CompAbility    *ability    = &world->abilities[id];
    AbilityData    *data       = &ability->stateData[ability->activeSlot];
    CompController *controller = &world->controllers[id];
    if (data->stage > 0)
    {
        vec4s       color                    = {0.0f, 1.0f, 0.0f, 1.0f};
        const char *hand                     = ability->activeSlot == 1 ? "hand.R" : "hand.L";
        vec3s       handPos                  = Sol_Model_GetBoneXform(world, id, hand).pos;
        float       widthScale               = Sol_Math_Lerp(0.2f, 2.5f, data->charge / 4.0f);
        data->as.laser.laserPointCountVisual = 0;
        Laser_Bounces_Visual(world, id, controller->aimpos, controller->aimdir, 0);
        for (int l = 0; l < data->as.laser.laserPointCountVisual; l += 2)
        {
            vec3s posA = l == 0 ? handPos : data->as.laser.laserPointsVisual[l];
            vec3s posB = data->as.laser.laserPointsVisual[l + 1];
            Draw_LaserStart(posA, color, 0.5f * widthScale, time);
            Draw_LaserStart(posA, VEC4_WHITE, 0.3f * widthScale, time);
            Draw_LaserImpact(posB, color, 1.0f * widthScale, time);
            Draw_LaserImpact(posB, VEC4_WHITE, 0.5f * widthScale, time);
            Draw_Laser(posA, posB, time, VEC4_BLACK, 0.4f * widthScale);
            // Draw_Laser(posA, posB, time, (vec4s){0.5f, 0.0f, 0.0f, 1.0f}, 0.4f * widthScale);
            Draw_Laser(posA, posB, time, color, 0.3f * widthScale);
            Draw_Laser(posA, posB, time, VEC4_WHITE, 0.2f * widthScale);
        }
    }
}

static void Laser_Bounces_Visual(World *world, int id, vec3s pos, vec3s dir, int bounceCount)
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

        Laser_Bounces_Visual(world, id, biasedOrigin, Sol_BounceVec(dir, result.norm), bounceCount + 1);
    }
}

static const vec2s sprite_section[4] = {
    {0.0f, 0.0f},
    {0.0f, 0.5f},
    {0.5f, 0.0f},
    {0.5f, 0.5f},
};
static void Draw_LaserImpact(vec3s pos, vec4s color, float scale, double time)
{
    u32       frame_index = ((u32)(time * 15.0)) % 4;
    vec2s     uv          = sprite_section[frame_index];
    QuadSSBO *ssbo        = Sol_Render_GetNext_Quad(QUADKIND_SPRITE_FRONT);
    ssbo->textureId       = SOL_TEXTURE_IMPACT;
    ssbo->color           = color;
    ssbo->pos             = (vec4s){pos.x, pos.y, pos.z, scale};
    ssbo->uv              = (vec4s){uv.x, uv.y, 0.5f, 0.5f};
    ssbo->rect            = GLMS_VEC4_ZERO;
    ssbo->extra           = GLMS_VEC4_ZERO;
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
    ssbo->rect            = GLMS_VEC4_ZERO;
    ssbo->extra           = GLMS_VEC4_ZERO;
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