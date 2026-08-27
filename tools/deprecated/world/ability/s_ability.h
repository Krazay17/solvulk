#pragma once
#include "sol/types.h"
#include "world.h"

// typedef struct
// {
//     union {
//         struct
//         {
//             vec3s     enterDir;
//             StrafeDir strafe;
//         } dash;
//         struct
//         {
//             vec3s laserPoints[16];
//             int   laserPointCount;
//             vec3s laserPointsVisual[16];
//             int   laserPointCountVisual;
//         } laser;
//         struct
//         {
//             vec3s whipPoints[16];
//             int   whipPointCount;
//         } whip;
//     } as;

//     AbilityState kind;
//     float        elapsed, accum, power, recover;
//     float        duration;
//     double       lastEntered, lastExited;
//     u32          stage;
//     u32          hitSessionGen;
//     bool         held, doesHit;
// } AbilityStateData;

typedef struct CompAbility
{
    int              state, activeSlot;
    int              action_map[ABILITY_SLOTS];
    AbilityStateData stateData[ABILITY_SLOTS];
} CompAbility;

typedef struct
{
    int ability_map[ABILITY_SLOTS];
} AbilityDesc;

const char                *ability_names[];
extern const AbilityConfig ability_base[];

void         Sol_Ability_Init(World *world);
CompAbility *Sol_Ability_Add(World *world, int id, AbilityDesc desc);
CompAbility *Sol_Ability_Get(World *world, int id);
bool         Sol_Ability_Has(World *world, int id);
void         Sol_Ability_Rem(World *world, int id);

AbilityStateData *Sol_Ability_GetActiveSlotData(World *world, int id);
bool              Sol_Ability_SetState(World *world, int id, AbilityState nextState, int slot, bool force);
