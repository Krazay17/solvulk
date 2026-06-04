#pragma once
#include "sol/types.h"

typedef struct CompBody2d CompBody2d;

typedef void (*Resolver)(World *world, vec2s *posA, CompBody2d *bodyA, vec2s *posB, CompBody2d *bodyB);
void ResolveRect(World *world, vec2s *posA, CompBody2d *bodyA, vec2s *posB, CompBody2d *bodyB);

extern Resolver resolver_kinds[BODY2DKIND_COUNT];

static inline vec2s ApplyFriction2(vec2s wishdir, vec2s prevvel, float friction, float dt)
{
    const float speed = glms_vec2_norm(prevvel);
    if (speed < 0.025f)
        return GLMS_VEC2_ZERO;
    vec2s       vel      = prevvel;
    const float drop     = speed * friction * dt;
    const float newspeed = fmaxf(0.0f, speed - drop);
    vel                  = glms_vec2_scale(vel, newspeed / speed);

    return vel;
}

void Grab(vec2s *vel, vec2s pos, vec2s dims);
void CollideScreenEdge(vec2s *vel, vec2s *pos, vec2s dims);
bool IsOverlappingRect(World *world, int idA, int idB);