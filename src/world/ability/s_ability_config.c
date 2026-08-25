#include "si_ability.h"
#include "buff/s_buff.h"
#include "combat/s_combat.h"

const char *ability_names[ABILITY_STATE_COUNT] = {
    [ABILITY_STATE_IDLE] = "Idle",           [ABILITY_STATE_DASH] = "Dash",
    [ABILITY_STATE_FIREBALL] = "Fireball",   [ABILITY_STATE_PISTOL] = "Pistol",
    [ABILITY_STATE_SPINSLASH] = "Spinslash", [ABILITY_STATE_CLAW] = "Claw",
    [ABILITY_STATE_SHIELD] = "Shield",       [ABILITY_STATE_LASER] = "Laser",
    [ABILITY_STATE_WHIP] = "Whip",           [ABILITY_STATE_FIREBALLVOLLEY] = "FireballVolley",
};

const AbilityConfig ability_base[ABILITY_STATE_COUNT] = {
    [ABILITY_STATE_IDLE] =
        {
            0,
        },
    [ABILITY_STATE_DASH] =
        {
            .duration   = 0.3f,
            .cooldown   = 0.0f,
            .damage     = 10.0f,
            .effectMask = EFFECTMASK_KNOCKBACK,
            .buffMask   = BUFFKIND_FIRE,
            .maxpower   = 4.0f,
        },
    [ABILITY_STATE_FIREBALL] =
        {
            .duration   = 3.0f,
            .cooldown   = 0.0f,
            .damage     = 10.0f,
            .effectMask = EFFECTMASK_KNOCKBACK,
            .buffMask   = BUFFKIND_FIRE,
            .maxpower   = 2.0f,
        },
    [ABILITY_STATE_PISTOL] =
        {
            .duration   = 0.5f,
            .cooldown   = 0.0f,
            .damage     = 10.0f,
            .effectMask = EFFECTMASK_KNOCKBACK,
            .buffMask   = BUFFKIND_FIRE,
            .maxpower   = 4.0f,
        },
    [ABILITY_STATE_SPINSLASH] =
        {
            .duration   = 0.5f,
            .cooldown   = 0.0f,
            .damage     = 10.0f,
            .effectMask = EFFECTMASK_KNOCKBACK,
            .buffMask   = BUFFKIND_FIRE,
            .maxpower   = 4.0f,
        },
    [ABILITY_STATE_CLAW] =
        {
            .duration   = 0.5f,
            .cooldown   = 0.0f,
            .damage     = 10.0f,
            .effectMask = EFFECTMASK_KNOCKBACK,
            .buffMask   = BUFFKIND_FIRE,
            .maxpower   = 4.0f,
        },
    [ABILITY_STATE_SHIELD] =
        {
            .duration   = 0.5f,
            .cooldown   = 0.0f,
            .damage     = 10.0f,
            .effectMask = EFFECTMASK_KNOCKBACK,
            .buffMask   = BUFFKIND_FIRE,
            .maxpower   = 4.0f,
        },
    [ABILITY_STATE_LASER] =
        {
            .duration   = 0.5f,
            .cooldown   = 0.0f,
            .damage     = 10.0f,
            .effectMask = EFFECTMASK_KNOCKBACK,
            .buffMask   = BUFFKIND_FIRE,
            .maxpower   = 4.0f,
        },
    [ABILITY_STATE_WHIP] =
        {
            .duration   = 0.5f,
            .cooldown   = 0.0f,
            .damage     = 10.0f,
            .effectMask = EFFECTMASK_KNOCKBACK,
            .buffMask   = BUFFKIND_FIRE,
            .maxpower   = 4.0f,
        },
    [ABILITY_STATE_FIREBALLVOLLEY] =
        {
            .duration   = 0.5f,
            .cooldown   = 0.0f,
            .damage     = 10.0f,
            .effectMask = EFFECTMASK_KNOCKBACK,
            .buffMask   = BUFFKIND_FIRE,
            .maxpower   = 4.0f,
        },
};

const AbilityConfig ability_rarity_base[ABILITY_STATE_COUNT][4] =
    {
        [ABILITY_STATE_FIREBALL][0] =
            {
                .cooldown = 5.0f,
                .damage   = 10,
            },
        [ABILITY_STATE_FIREBALL][1] =
            {
                .cooldown = 4.0f,
                .damage   = 15,
                .buffMask = BITC(BUFFKIND_FIRE),
            },
        [ABILITY_STATE_FIREBALL][2] =
            {
                .cooldown   = 2.0f,
                .damage     = 20,
                .buffMask   = BITC(BUFFKIND_FIRE),
                .effectMask = EFFECTMASK_CHAINLIGHTNING | EFFECTMASK_KNOCKBACK,
            },
        [ABILITY_STATE_SHIELD][0] =
            {
                .cooldown   = 12.0f,
                .damage     = 10,
                .duration   = 0.25f,
                .effectMask = EFFECTMASK_KNOCKBACK,
            },
        [ABILITY_STATE_SHIELD][1] =
            {
                .cooldown   = 10.0f,
                .damage     = 10,
                .duration   = 0.25f,
                .effectMask = EFFECTMASK_KNOCKBACK | EFFECTMASK_REFLECTPROJECTILE,
            },
        [ABILITY_STATE_SHIELD][2] =
            {
                .cooldown   = 8.0f,
                .damage     = 15,
                .duration   = 0.5f,
                .effectMask = EFFECTMASK_KNOCKBACK | EFFECTMASK_KNOCKUP | EFFECTMASK_REFLECTPROJECTILE |
                              EFFECTMASK_CHAINLIGHTNING,
            },
        [ABILITY_STATE_SPINSLASH][0] =
            {
                .cooldown = 12.0f,
                .duration = 0.5f,
                .damage   = 20,
            },
        [ABILITY_STATE_SPINSLASH][1] =
            {
                .cooldown   = 10.0f,
                .duration   = 0.5f,
                .damage     = 25,
                .effectMask = EFFECTMASK_KNOCKUP,
            },
        [ABILITY_STATE_SPINSLASH][2] =
            {
                .cooldown   = 8.0f,
                .duration   = 0.5f,
                .damage     = 25,
                .buffMask   = BITC(BUFFKIND_FIRE) | BITC(BUFFKIND_STUN),
                .effectMask = EFFECTMASK_KNOCKUP,
            },
        [ABILITY_STATE_SPINSLASH][3] =
            {
                .cooldown   = 2.0f,
                .duration   = 0.5f,
                .damage     = 25,
                .buffMask   = BITC(BUFFKIND_FIRE) | BITC(BUFFKIND_STUN),
                .effectMask = EFFECTMASK_KNOCKUP | EFFECTMASK_REFLECTPROJECTILE | EFFECTMASK_CHAINLIGHTNING |
                              EFFECTMASK_HEALONHIT | EFFECTMASK_KNOCKBACK,
            },
        [ABILITY_STATE_DASH][0] =
            {
                .cooldown = 2.0f,
                .duration = 0.2f,
            },
        [ABILITY_STATE_DASH][1] =
            {
                .cooldown = 1.5f,
                .duration = 0.3f,
            },
        [ABILITY_STATE_DASH][2] =
            {
                .cooldown   = 1.0f,
                .duration   = 0.5f,
                .damage     = 10,
                .buffMask   = BITC(BUFFKIND_STUN),
                .effectMask = EFFECTMASK_KNOCKBACK | EFFECTMASK_REFLECTPROJECTILE,
            },
        [ABILITY_STATE_PISTOL][0] =
            {
                .cooldown = 0.3f,
                .damage   = 5,
            },
        [ABILITY_STATE_PISTOL][1] =
            {
                .cooldown = 0.2f,
                .damage   = 8,
            },
        [ABILITY_STATE_PISTOL][2] =
            {
                .cooldown   = 0.1f,
                .damage     = 8,
                .buffMask   = BITC(BUFFKIND_FIRE),
                .effectMask = EFFECTMASK_CHAINLIGHTNING,
            },
        [ABILITY_STATE_CLAW][0] =
            {
                .cooldown = 1.5f,
                .duration = 0.5f,
                .damage   = 25,
            },
        [ABILITY_STATE_CLAW][1] =
            {
                .cooldown = 1.2f,
                .duration = 0.5f,

                .damage = 30,
            },
        [ABILITY_STATE_CLAW][2] =
            {
                .cooldown   = 0.8f,
                .duration   = 0.5f,
                .effectMask = EFFECTMASK_KNOCKBACK | EFFECTMASK_REFLECTPROJECTILE | EFFECTMASK_CHAINLIGHTNING |
                              EFFECTMASK_HEALONHIT,
                .damage     = 35,
            },
        [ABILITY_STATE_LASER][0] =
            {
                .cooldown = 2.0f,
                .damage   = 20,
            },
        [ABILITY_STATE_LASER][1] =
            {
                .cooldown = 2.0f,
                .damage   = 25,
            },
        [ABILITY_STATE_LASER][2] =
            {
                .cooldown   = 2.0f,
                .damage     = 30,
                .effectMask = EFFECTMASK_KNOCKBACK | EFFECTMASK_CHAINLIGHTNING,
            },

        [ABILITY_STATE_FIREBALLVOLLEY][0] =
            {
                .cooldown = 6.0f,
            },
};