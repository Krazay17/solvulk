#pragma once
#include "s_combat.h"

#define CHAIN_CAP 0xff

typedef struct Dmgnumber
{
    int   amnt;
    float ttl;
    vec3s pos;
    vec4s color;
} Dmgnumber;
typedef struct Dmgnumbers
{
    Dmgnumber *dmgNumber;
    int        count;
    int        cap;
} Dmgnumbers;

typedef struct
{
    u8    kind;
    float damage;
    int   count, dealer, last;
    int   hitEnts[65536];
    float accum, delay;
} Chain;
typedef struct ChainAttacks
{
    Chain *chains;
    u32    count;
    u32    capacity;
} ChainAttacks;

void Dmgnumbers_Draw(World *world, double dt, double time);
void Dmgnumbers_Step(World *world, double dt, double time);
void Chain_Step(World *world, double dt, double time);