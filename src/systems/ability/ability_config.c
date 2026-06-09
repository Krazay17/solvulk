#include "sol_core.h"

#include "ability_i.h"

AbilityConfig ability_config[ABILITY_STATE_COUNT][3] = {
    [ABILITY_STATE_FIREBALL][0] =
        {
            .name = "Fireball",
            .cooldown = 4.0f,
            .damage = 10,
        },
    [ABILITY_STATE_FIREBALL][1] =
        {
            .name = "Fireball",
            .cooldown = 2.0f,
            .damage = 15,
            .buffMask = BUFFMASK_FIRE,
        },
    [ABILITY_STATE_FIREBALL][2] =
        {
            .name = "Fireball",
            .cooldown = 1.0f,
            .damage = 20,
            .buffMask = BUFFMASK_FIRE,
        },
    [ABILITY_STATE_SHIELD][0] =
        {
            .name = "Shield",
            .duration = 0.5f,
            .cooldown = 8.0f,
        },
    [ABILITY_STATE_SHIELD][1] =
        {
            .name = "Shield",
            .duration = 0.5f,
            .cooldown = 6.0f,
        },
    [ABILITY_STATE_SHIELD][2] =
        {
            .name = "Shield",
            .duration = 1.0f,
            .cooldown = 5.0f,
        },
    [ABILITY_STATE_SPINSLASH][0] =
        {
            .name = "SpinSlash",
            .duration = 0.5f,
            .cooldown = 9.0f,
        },
    [ABILITY_STATE_SPINSLASH][1] =
        {
            .name = "SpinSlash",
            .duration = 0.5f,
            .cooldown = 8.0f,
        },
    [ABILITY_STATE_SPINSLASH][2] =
        {
            .name = "SpinSlash",
            .duration = 0.5f,
            .cooldown = 7.0f,
        },
    [ABILITY_STATE_DASH][0] =
        {
            .name = "Dash",
            .duration = 0.15f,
            .cooldown = 1.0f,
        },
    [ABILITY_STATE_DASH][1] =
        {
            .name = "Dash",
            .duration = 0.3f,
            .cooldown = 1.0f,
        },
    [ABILITY_STATE_DASH][2] =
        {
            .name = "Dash",
            .duration = 0.5f,
            .cooldown = 1.0f,
        },
    [ABILITY_STATE_PISTOL][0] =
        {
            .name = "Blaster",
            .cooldown = 0.3f,
            .damage   = 5,
        },
    [ABILITY_STATE_PISTOL][1] =
        {
            .name = "Blaster",
            .cooldown = 0.2f,
            .damage   = 7,
        },
    [ABILITY_STATE_PISTOL][2] =
        {
            .name = "Blaster",
            .cooldown = 0.15f,
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