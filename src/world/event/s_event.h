#pragma once
#include "sol/types.h"

typedef struct Emitter Emitter;

typedef enum
{
    FXKIND_FIREBALL_SHOOT = 1,
    FXKIND_FIREBALL_HIT,
    FXKIND_FIRE_APPLY,
    FXKIND_SHIELD_BURST,
    FXKIND_SHIELD_HIT,
    FXKIND_SPINHIT,
    FXKIND_BULLET_HIT,
    FXKIND_CHAINLIGHTNING,
    FXKIND_LIGHTNING,
    FXKIND_LASER_HIT,
    FXKIND_SWORD_HIT,
    FXKIND_SWORD_SWING,
    FXKIND_TAKEDAMAGE,
    FXKIND_DEATH_BLOOD,
    FXKIND_COUNT,
} FxKind;

typedef enum
{
    TRIGGERKIND_SPAWN_WIZARD,
} TriggerKind;

typedef enum
{
    EVENTKIND_COLLISION,
    EVENTKIND_FX,
    EVENTKIND_SOUND,
    EVENTKIND_EQUIP,
    EVENTKIND_SCORE,
    EVENTKIND_COUNT,
} EventKind;
typedef struct SolEvent
{
    EventKind kind;
    u32       entA, entB;
    union {
        struct
        {
            vec3s pos, normal, vel;
            u32   entA, entB;
        } collision;
        struct
        {
            float damage;
            u32   entA, entB;
        } death;
        struct
        {
            vec3s pos;
            vec4s color;
            u32   kind, entA, entB;
            float scale, duration;
        } fx;
        struct
        {
            u32   kind;
            vec3s pos;
            float volume;
        } sound;
        struct
        {
            TriggerKind kind;
            vec3s       pos;
        } trigger;
        struct
        {
            u32   ent;
            vec3s pos;
        } respawn;
        struct
        {
            u32 entId;
            u32 slot;
            u32 ability;
            u32 rarity;
        } equip;
        struct
        {
            u32   entA, entB;
            float damageDealt;
        } score;
    } as;
} SolEvent;

typedef struct SolEvents
{
    SolEvent *event;

    u32 count;
    u32 capacity;
} SolEvents;

void Sol_Event_Init(World *world);
void Sol_Event_HandleFx_Init(World *w);

void Sol_Event_Add(World *world, SolEvent event);
void Sol_Events_Clear(World **worlds, int count);
void Sol_Event_Clear(World *world);