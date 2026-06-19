#pragma once
#include "sol/types.h"

typedef struct World World;

typedef enum
{
    FXKIND_NONE,
    FXKIND_FIREBALL_SHOOT,
    FXKIND_FIREBALL_HIT,
    FXKIND_FIRE_APPLY,
    FXKIND_SHIELD_BURST,
    FXKIND_SHIELD_HIT,
    FXKIND_SPINHIT,
    FXKIND_BULLET_HIT,
    FXKIND_CHAINLIGHTNING,
    FXKIND_SWORD_HIT,
    FXKIND_SWORD_SWING,
    FXKIND_TAKEDAMAGE,
    FXKIND_DEATH_BLOOD,
} FxKind;

typedef enum
{
    EVENTKIND_COLLISION,
    EVENTKIND_HIT,
    EVENTKIND_FX,
    EVENTKIND_DEATH,
    EVENTKIND_RESPAWN,
    EVENTKIND_EQUIP,
    EVENTKIND_COUNT,
} EventKind;
typedef enum
{
    TRIGGERKIND_SPAWN_WIZARD,
} TriggerKind;
typedef struct SolEvent
{
    EventKind kind;
    union {
        SolHit hit;
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
            u32   entA, entB;
            vec3s pos, rot;
            u32   kind;
            float scale;
        } fx;
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
    } as;
} SolEvent;

typedef struct SolEvents
{
    SolEvent *event;

    u32 count;
    u32 capacity;
} SolEvents;

void Sol_Event_Add(World *world, SolEvent event);
void Sol_Events_Clear(World **worlds, int count);
void Sol_Event_Init(World *world);
void Sol_Event_Clear(World *world);