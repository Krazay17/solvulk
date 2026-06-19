#pragma once
#include "types.h"

typedef struct World World;

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

    SolHit directHit;
    SolHit explosionHit;
} CompProjectile;

void            Sol_Projectile_Init(World *world);
CompProjectile *Sol_Projectile_Add(World *world, int id, ProjectileKind kind, float power);
