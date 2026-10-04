/*
 * File: ability_bolt.c
 * Author: Josh Massarella
 * GitHub: https://github.com/Krazay17
 * Created: 2026-10-01
 * Throw lightning bolt on charge, on bolt hit pull guy to bolt if charging.
 * Pickup bolt if touch bolt while charging.
 * On fire pull bolt if not picked up, else throw bolt.
 */
#include "ability/s_ability.h"
#include "world.h"
#include "sol_math.h"
#include "estate.h"
#include "prefabs.h"
#include "render/render.h"

#define MIN_POWER 0.2f

const float bolt_speed = 80.0f;
const float accel      = 12.0f; // speed gained per second
const float min_speed  = 12.0f; // min grapple speed

const float panspeed           = 6.0f; // texure pan
const float unwrap_distance_sq = 2.0f;
const float wrap_distance_sq   = 4.0f;

static float GetBoltD2(World *world, vec3s pos, AbilityStateData *data)
{
    vec3s bolt_pos = world->xform.pos[data->as.bolt.bolt];
    vec3s delta    = glms_vec3_sub(bolt_pos, pos);
    return glms_vec3_dot(delta, delta);
}

static void RetractAnchor(World *world, int id, vec3s shoot_pos, AbilityStateData *data)
{
    for (int i = 0; i < data->as.bolt.current_anchor; i++)
    {
        vec3s anchor = data->as.bolt.anchor[i];
        vec3s delta  = glms_vec3_sub(anchor, shoot_pos);
        float d2     = glms_vec3_dot(delta, delta);
        if (d2 < 0.001f)
            return;
        float dist          = sqrt(d2);
        vec3s dir           = glms_vec3_scale(delta, 1.0f / dist);
        SolRayResult result = {0};
        bool hit            = Sol_Raycast1(world,
                                           (SolRay){
                                               .start     = shoot_pos,
                                               .dir       = dir,
                                               .dist      = dist,
                                               .ignoreEnt = id,
                                               .mask      = COLLAYER_ALL,
                                           },
                                           &result);
        float d2_hit        = glms_vec3_norm2(glms_vec3_sub(result.pos, anchor));
        if (!hit || (d2_hit < unwrap_distance_sq))
        {
            data->as.bolt.current_anchor = i;
            break;
        }
    }
}

static bool FindAnchor(World *world, int id, vec3s shoot_pos, AbilityStateData *data, vec3s *out_to_anchor)
{
    RetractAnchor(world, id, shoot_pos, data);
    data->as.bolt.anchor[0] = world->xform.pos[data->as.bolt.bolt];
    vec3s delta             = glms_vec3_sub(data->as.bolt.anchor[data->as.bolt.current_anchor], shoot_pos);
    float d2                = glms_vec3_dot(delta, delta);
    if (d2 < 0.001f)
        return false;
    float dist          = sqrt(d2);
    vec3s dir           = glms_vec3_scale(delta, 1.0f / dist);
    SolRayResult result = {0};
    bool hit            = Sol_Raycast1(world,
                                       (SolRay){
                                           .start     = shoot_pos,
                                           .dir       = dir,
                                           .dist      = dist,
                                           .ignoreEnt = id,
                                           .mask      = COLLAYER_ALL,
                                       },
                                       &result);
    if (hit && data->as.bolt.current_anchor < (MAX_BOLT_ANCHORS - 1))
    {
        float d2_hit = glms_vec3_norm2(glms_vec3_sub(result.pos, data->as.bolt.anchor[data->as.bolt.current_anchor]));
        if (d2_hit > wrap_distance_sq)
        {
            data->as.bolt.anchor[++data->as.bolt.current_anchor] = result.pos;
        }
    }
    *out_to_anchor = glms_vec3_normalize(glms_vec3_sub(data->as.bolt.anchor[data->as.bolt.current_anchor], shoot_pos));

    return hit;
}

static void BoltDelete(World *world, AbilityStateData *data)
{
    if (data->as.bolt.bolt > 0)
    {
        Sol_Destroy_Ent(world, data->as.bolt.bolt);
        data->as.bolt.bolt       = 0;
        data->as.bolt.bolt_state = 0;
    }
}

static void BoltHit(World *world, int a, int b)
{
    ScRef *ref = Sol_Comp_Get(world, a, ScRef);
    if (!ref)
        return;
    ScAbility *ability = Sol_Comp_Get(world, ref->ent_id, ScAbility);
    if (!ability)
        return;

    sollog(a);
    AbilityStateData *data   = &ability->stateData[ref->index];
    data->as.bolt.anchor[0]  = world->xform.pos[data->as.bolt.bolt];
    data->as.bolt.bolt_state = 1;
}

static void Charge(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot, float dt)
{
    AbilityStateData *data = &ability->stateData[slot];
    switch (data->stage)
    {
    case 0:
        if (!data->held)
        {
            data->stage++;
            ability->prio_slot = slot;
            if (data->as.bolt.bolt_state == 2)
            {
                Sol_Event_Push(world, EVENTKIND_FX,
                               (SolEvent){.as.fx.kind = FXKIND_SHOOT, .as.fx.pos = world->xform.pos[id]});
                BoltDelete(world, data);
                int bolt = Sol_Prefab_LightningBolt(world, id, Sol_Body3_GetHead(world, id), cmd->aimdir, bolt_speed,
                                                    0.33f, NULL);
                ScTimer *timer   = Sol_Comp_Add(world, bolt, ScTimer);
                timer->duration  = 2.0f;
                timer->destroy   = true;
                timer->shrinkout = true;
                ScProjectile *p  = Sol_Comp_Get(world, bolt, ScProjectile);
                p->hit.damage    = data->conf.damage;
                p->hit.power     = data->power;
            }
            else
            {
                Sol_Comp_Rem(world, data->as.bolt.bolt, ScParent);
                data->as.bolt.bolt_state = 3;
            }
        }

        data->power = minf(data->conf.maxpower, data->power + dt * data->conf.speed);

        switch (data->as.bolt.bolt_state)
        {
        case 1: {
            vec3s pos = Sol_Body3_GetHead(world, id);
            float d2  = GetBoltD2(world, pos, data);
            vec3s to_anchor;
            FindAnchor(world, id, pos, data, &to_anchor);

            ScBody3 *body3 = Sol_Comp_Get(world, id, ScBody3);
            ScMove3 *move3 = Sol_Comp_Get(world, id, ScMove3);
            if (body3 && move3)
            {
                move3->frictionMod = 0.0f;
                vec3s look         = cmd->lookdir;
                float align        = glms_vec3_dot(to_anchor, look);
                vec3s axis         = glms_vec3_cross(to_anchor, look);
                vec3s tangent      = glms_vec3_cross(axis, to_anchor);
                vec3s v_side =
                    glms_vec3_sub(body3->vel, glms_vec3_scale(to_anchor, glms_vec3_dot(body3->vel, to_anchor)));
                tangent = glms_vec3_norm2(v_side) > 0.0001f ? glms_vec3_normalize(v_side) : to_anchor;

                float t = fabs(align);
                t       = t * t * t;
                t       = max(0.2f, t);

                vec3s target_dir = glms_vec3_normalize(glms_vec3_lerpc(tangent, to_anchor, t));
                float speed      = glms_vec3_norm(body3->vel);
                speed            = fmaxf(speed, min_speed) + accel * dt;
                body3->vel       = glms_vec3_scale(target_dir, speed);
            }

            if (d2 < 10.0f)
            {
                BoltDelete(world, data);
                data->as.bolt.bolt_state = 2;
            }
        }
        break;
        }

        break;
    case 1: {
        if (data->as.bolt.bolt_state == 3)
        {
            vec3s pos = Sol_Body3_GetHead(world, id);
            if (GetBoltD2(world, pos, data) < 0.5f)
            {
                BoltDelete(world, data);
                data->as.bolt.bolt_state = 4;
            }
            ScBody3 *body3 = Sol_Comp_Get(world, data->as.bolt.bolt, ScBody3);
            if (body3)
            {
                vec3s to_player = glms_vec3_normalize(
                    glms_vec3_sub(Sol_Body3_GetHead(world, id), world->xform.pos[data->as.bolt.bolt]));
                body3->vel = glms_vec3_scale(to_player, bolt_speed);
            }
        }
    }
    break;
    }
    if (data->stage > 0)
    {
        data->elapsed += dt;
        if ((data->elapsed >= data->conf.duration) && data->as.bolt.bolt_state != 3)
        {
            Sol_Ability_SetState(world, id, 0, slot, true);
        }
    }
}

static void Enter(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot)
{
    AbilityStateData *data = &ability->stateData[slot];
    data->conf             = Sol_Ability_GetSlotConf(ability, slot);

    switch (slot_kind_map[slot])
    {
    case 0:
        data->power = data->conf.maxpower;
        break;
    case 1:
        data->power = MIN_POWER;
        break;
    }

    int bolt =
        Sol_Prefab_LightningBolt(world, id, Sol_Body3_GetHead(world, id), cmd->aimdir, bolt_speed, 0.33f, BoltHit);
    ScRef *ref                   = Sol_Comp_Add(world, bolt, ScRef);
    ref->kind                    = REFKIND_ABILITY;
    ref->index                   = slot;
    ref->ent_id                  = id;
    data->as.bolt.bolt           = bolt;
    data->as.bolt.bolt_state     = 0;
    data->as.bolt.current_anchor = 0;
    ScProjectile *p              = Sol_Comp_Get(world, bolt, ScProjectile);
    p->hit.damage                = data->conf.damage;
    p->hit.power                 = data->power;

    Sol_Event_Push(world, EVENTKIND_FX, (SolEvent){.as.fx.kind = FXKIND_SHOOT, .as.fx.pos = world->xform.pos[id]});
}
static void Exit(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot)
{
    AbilityStateData *data  = &ability->stateData[slot];
    data->cooldownRemaining = data->conf.cooldown;

    BoltDelete(world, data);
}
static bool CanExit(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot)
{
    return true;
}
static bool CanEnter(World *world, int id, ScAbility *ability, ScCmd *cmd, int slot)
{
    AbilityStateData *data = &ability->stateData[slot];
    return !(data->cooldownRemaining > 0.0f);
}
static DefendResult Defend(World *world, int id, ScAbility *ability, int slot, SolHit *hit)
{
}
static void Draw(World *world, int id, ScAbility *ability, int slot, float dt)
{
    AbilityStateData *data = &ability->stateData[slot];
    vec3s hand_pos         = Sol_Model_GetBoneXform(world, id, slot > 5 ? "hand.R" : "hand.L").pos;
    vec3s bolt_pos         = world->xform.pos[data->as.bolt.bolt];
    switch (data->stage)
    {
    case 0: {
        switch (data->as.bolt.bolt_state)
        {
        case 1: {
            vec4s hand_pos4 = {hand_pos.x, hand_pos.y, hand_pos.z, 0.1f};
            vec4s anchor4   = {bolt_pos.x, bolt_pos.y, bolt_pos.z, 0.3f};
            for (int i = 1; i < data->as.bolt.current_anchor + 1; i++)
            {
                vec3s prev_anchor = data->as.bolt.anchor[i - 1];
                vec3s anchor3     = data->as.bolt.anchor[i];
                anchor4           = (vec4s){anchor3.x, anchor3.y, anchor3.z, 0.3f};
                Sol_Draw_Lightning(world, dt,
                                   &(Ribbon){
                                       .points[0]    = prev_anchor,
                                       .points[1]    = anchor3,
                                       .thickness    = 0.3f,
                                       .texture      = SOL_TEXTURE_LIGHTNING,
                                       .color        = VEC4_WHITE,
                                       .pan          = 6.0f,
                                       ._point_count = 2,
                                   });
            }
            Sol_Draw_Lightning(world, dt,
                               &(Ribbon){
                                   .points[0]    = hand_pos,
                                   .points[1]    = (vec3s){anchor4.x, anchor4.y, anchor4.z},
                                   .thickness    = 0.1f,
                                   .texture      = SOL_TEXTURE_LIGHTNING,
                                   .color        = VEC4_WHITE,
                                   .pan          = 6.0f,
                                   ._point_count = 2,
                               });
        }
        break;
        case 2: {
            vec4s hand_pos4 = {hand_pos.x, hand_pos.y, hand_pos.z, data->power * 1.0f};
            vec3s fwd       = Sol_Comp_Get(world, id, ScCmd)->lookdir;
            vec4s rot4      = Sol_Rot_FromVecs(fwd, WORLD_UP);

            vec4s model_sca                             = {0.5f, 0.0f, 0.5f, 1.0f};
            model_sca.y                                 = data->power * 1.0f;
            *Sol_Render_GetNextModel(0, MODELKIND_BOLT) = (ModelSSBO){
                .color    = {1.0f, 1.0f, 1.0f, 1.0f},
                .position = hand_pos4,
                .scale    = model_sca,
                .rotation = rot4,
            };

            versors roll_90   = glms_quatv(GLM_PI_2f, (vec3s){0.0f, 1.0f, 0.0f});
            versors cross_rot = glms_quat_mul(to_versors(rot4), roll_90);
            for (int i = 0; i < 2; i++)
            {
                QuadSSBO *quad  = Sol_Render_GetNextQuad(PIPE_QUAD_ADD);
                quad->textureId = SOL_TEXTURE_SHOCKSPRITE4;
                quad->type      = 1;
                quad->pos       = hand_pos4;
                quad->rot       = i == 0 ? rot4 : to_vec4s(cross_rot);
                quad->color     = VEC4_WHITE;
                quad->rect      = (vec4s){0, 0, 1.0f, 3.0f};
                u32 sprite_anim = (u32)floorf(world->tickTime * 10.0f) % 4;
                quad->uv        = SPRITEPAGE4[sprite_anim];
            }
        }
        break;
        }
    }
    break;
    }
}

const AbilityStateFunc ability_bolt_charge = {Charge, Enter, Exit, CanExit, CanEnter, Draw, Defend};