#pragma once
#include "sol/types.h"

#include "combat/combat_types.h"

typedef enum
{
    CONTACTKIND_FIREBALL,
    CONTACTKIND_WIZARDTOUCH,
    CONTACTKIND_BULLET,
    CONTACTKIND_COUNT,
} ContactKind;
typedef struct CompContact
{
    ContactKind kind;
    u32         bounces;
    float       damageScale, radiusScale;
} CompContact;
void Sol_Contact_Init(World *world);

void Sol_Contact_Add(World *world, int id, ContactKind kind, float damageScale);
void Sol_Contact_SetBounces(World *world, int id, int bounces);
void Sol_Contact_SetRadiusScale(World *world, int id, float radiusScale);