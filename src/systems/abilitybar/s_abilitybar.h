#pragma once
#include "sol/types.h"
#include "components.h"

// Fills bar-relative rects (x, y, w, h). Returns slot count.
// frames[g].z == 0 means the group is empty.
static inline int Abilitybar_Layout(const ScAbilitybar *b, vec4s slots[ABILITY_SLOTS], vec4s frames[ABILITY_GROUPS])
{
    int n   = 0;
    float x = 0.0f;
    float h = b->slot_dims.y + b->frame_pad.y * 2.0f;

    for (int g = 0; g < ABILITY_GROUPS; g++)
    {
        int cnt   = b->slots_per_group[g];
        frames[g] = (vec4s){x, 0.0f, 0.0f, h};
        if (cnt <= 0)
            continue;

        float w   = cnt * b->slot_dims.x + (cnt - 1) * b->spacing + b->frame_pad.x * 2.0f;
        frames[g] = (vec4s){x, 0.0f, w, h};

        for (int i = 0; i < cnt && n < ABILITY_SLOTS; i++)
            slots[n++] = (vec4s){x + b->frame_pad.x + i * (b->slot_dims.x + b->spacing),
                                 b->frame_pad.y, b->slot_dims.x, b->slot_dims.y};
        x += w + b->group_gap;
    }
    return n;
}

static inline vec2s Abilitybar_Size(const ScAbilitybar *b)
{
    vec4s slots[ABILITY_SLOTS], frames[ABILITY_GROUPS];
    Abilitybar_Layout(b, slots, frames);
    float right = 0.0f;
    for (int g = 0; g < ABILITY_GROUPS; g++)
        if (frames[g].z > 0.0f)
            right = frames[g].x + frames[g].z;
    return (vec2s){right, b->slot_dims.y + b->frame_pad.y * 2.0f};
}