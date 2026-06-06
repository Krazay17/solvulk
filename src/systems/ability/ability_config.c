#include "sol_core.h"

#include "ability_i.h"

AbilityConfig ability_config[ABILITY_STATE_COUNT] = {
    [ABILITY_STATE_SPINSLASH] =
        {
            .cooldown = 5.0f,
        },
    [ABILITY_STATE_SHIELD] =
        {
            .cooldown = 8.0f,
        },
    [ABILITY_STATE_IDLE] =
        {
            .cooldown = 0,
        },
    [ABILITY_STATE_DASH] =
        {
            .cooldown = 1.0f,
        },
    [ABILITY_STATE_FIREBALL] =
        {
            .cooldown = 1.0f,
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