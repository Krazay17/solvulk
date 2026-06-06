#include "sol_core.h"

#include "ability_i.h"

AbilityConfig ability_config[ABILITY_STATE_COUNT] = {
    [ABILITY_STATE_SPINSLASH] =
        {
            .duration = 0.5f,
            .cooldown = 9.0f,
        },
    [ABILITY_STATE_SHIELD] =
        {
            .duration = 0.5f,
            .cooldown = 8.0f,
        },
    [ABILITY_STATE_DASH] =
        {
            .duration = 0.5f,
            .cooldown = 1.0f,
        },
    [ABILITY_STATE_FIREBALL] =
        {
            .cooldown = 4.0f,
        },
    [ABILITY_STATE_PISTOL] =
        {
            .cooldown = 0.1f,
        },
    [ABILITY_STATE_CLAW] =
        {
            .cooldown = 1.0f,
        },
    [ABILITY_STATE_FIREBALLVOLLEY] =
        {
            .cooldown = 6.0f,
        },

};