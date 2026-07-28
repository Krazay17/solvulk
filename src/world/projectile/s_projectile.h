#pragma once
#include "types.h"

typedef struct HitCallbackData
{
    World *world;
    vec3s  pos;
    float  scale;
    u32    instigator;
} HitCallbackData;
typedef enum
{
    PROJECTILEKIND_BULLET,
    PROJECTILEKIND_FIREBALL,
    PROJECTILEKIND_COUNT,
} ProjectileKind;
typedef struct CompProjectile
{
    ProjectileKind kind;
    u32            bounces;
    float          power;
    float          explodeRadius;
    u32            hitFX, explodeHitFX;

    SolCallback callback;
    u32         callbackFlags;
    SolHit      directHit;
    SolHit      explosionHit;
} CompProjectile;

void            Sol_Projectile_Init(World *world);
CompProjectile *Sol_Projectile_Add(World *world, int id, ProjectileKind kind, float power);
