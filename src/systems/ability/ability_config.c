#include "sol_core.h"

#include "ability_i.h"

AbilityConfig ability_config[ABILITY_STATE_COUNT][3] = {
    [ABILITY_STATE_FIREBALL][0] =
        {
            .cooldown = 4.0f,
        },
    [ABILITY_STATE_FIREBALL][1] =
        {
            .cooldown = 2.0f,
        },
    [ABILITY_STATE_FIREBALL][2] =
        {
            .cooldown = 1.0f,
        },
    [ABILITY_STATE_SHIELD][0] =
        {
            .duration = 0.5f,
            .cooldown = 8.0f,
        },
    [ABILITY_STATE_SHIELD][1] =
        {
            .duration = 0.5f,
            .cooldown = 6.0f,
        },
    [ABILITY_STATE_SHIELD][2] =
        {
            .duration = 1.0f,
            .cooldown = 5.0f,
        },
    [ABILITY_STATE_SPINSLASH][0] =
        {
            .duration = 0.5f,
            .cooldown = 9.0f,
        },
    [ABILITY_STATE_SPINSLASH][1] =
        {
            .duration = 0.5f,
            .cooldown = 8.0f,
        },
    [ABILITY_STATE_SPINSLASH][2] =
        {
            .duration = 0.5f,
            .cooldown = 7.0f,
        },
    [ABILITY_STATE_DASH][0] =
        {
            .duration = 0.15f,
            .cooldown = 1.0f,
        },
    [ABILITY_STATE_DASH][1] =
        {
            .duration = 0.3f,
            .cooldown = 1.0f,
        },
    [ABILITY_STATE_DASH][2] =
        {
            .duration = 0.5f,
            .cooldown = 1.0f,
        },
    [ABILITY_STATE_PISTOL][0] =
        {
            .cooldown = 0.3f,
            .fireRate = 0.3f,
            .damage   = 5,
        },
    [ABILITY_STATE_PISTOL][1] =
        {
            .cooldown = 0.2f,
            .fireRate = 0.2f,
            .damage   = 7,
        },
    [ABILITY_STATE_PISTOL][2] =
        {
            .cooldown = 0.15f,
            .fireRate = 0.15f,
            .damage   = 10,
        },
    [ABILITY_STATE_CLAW][0] =
        {
            0,
        },
    [ABILITY_STATE_FIREBALLVOLLEY][0] =
        {
            .cooldown = 6.0f,
        },

};