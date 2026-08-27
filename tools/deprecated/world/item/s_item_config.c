#include "s_item.h"

const AbilityConfig item_rarity_base[ABILITY_STATE_COUNT][4] =
    {
        [ABILITY_STATE_FIREBALL][0] =
            {
                .state    = ABILITY_STATE_FIREBALL,
                .cooldown = 5.0f,
                .damage   = 10,
            },
        [ABILITY_STATE_FIREBALL][1] =
            {
                .state    = ABILITY_STATE_FIREBALL,
                .cooldown = 4.0f,
                .damage   = 15,
                .buffMask = BITC(BUFFKIND_FIRE),
            },
        [ABILITY_STATE_FIREBALL][2] =
            {
                .state      = ABILITY_STATE_FIREBALL,
                .cooldown   = 2.0f,
                .damage     = 20,
                .buffMask   = BITC(BUFFKIND_FIRE),
                .effectMask = EFFECTMASK_CHAINLIGHTNING | EFFECTMASK_KNOCKBACK,
            },
        [ABILITY_STATE_SHIELD][0] =
            {
                .state      = ABILITY_STATE_SHIELD,
                .cooldown   = 12.0f,
                .damage     = 10,
                .duration   = 0.25f,
                .effectMask = EFFECTMASK_KNOCKBACK,
            },
        [ABILITY_STATE_SHIELD][1] =
            {
                .state      = ABILITY_STATE_SHIELD,
                .cooldown   = 10.0f,
                .damage     = 10,
                .duration   = 0.25f,
                .effectMask = EFFECTMASK_KNOCKBACK | EFFECTMASK_REFLECTPROJECTILE,
            },
        [ABILITY_STATE_SHIELD][2] =
            {
                .state      = ABILITY_STATE_SHIELD,
                .cooldown   = 8.0f,
                .damage     = 15,
                .duration   = 0.5f,
                .effectMask = EFFECTMASK_KNOCKBACK | EFFECTMASK_KNOCKUP | EFFECTMASK_REFLECTPROJECTILE |
                              EFFECTMASK_CHAINLIGHTNING,
            },
        [ABILITY_STATE_SPINSLASH][0] =
            {
                .state    = ABILITY_STATE_SPINSLASH,
                .cooldown = 12.0f,
                .duration = 0.5f,
                .damage   = 20,
            },
        [ABILITY_STATE_SPINSLASH][1] =
            {
                .state      = ABILITY_STATE_SPINSLASH,
                .cooldown   = 10.0f,
                .duration   = 0.5f,
                .damage     = 25,
                .effectMask = EFFECTMASK_KNOCKUP,
            },
        [ABILITY_STATE_SPINSLASH][2] =
            {
                .state      = ABILITY_STATE_SPINSLASH,
                .cooldown   = 8.0f,
                .duration   = 0.5f,
                .damage     = 25,
                .buffMask   = BITC(BUFFKIND_FIRE) | BITC(BUFFKIND_STUN),
                .effectMask = EFFECTMASK_KNOCKUP,
            },
        [ABILITY_STATE_SPINSLASH][3] =
            {
                .state      = ABILITY_STATE_SPINSLASH,
                .cooldown   = 2.0f,
                .duration   = 0.5f,
                .damage     = 25,
                .buffMask   = BITC(BUFFKIND_FIRE) | BITC(BUFFKIND_STUN),
                .effectMask = EFFECTMASK_KNOCKUP | EFFECTMASK_REFLECTPROJECTILE | EFFECTMASK_CHAINLIGHTNING |
                              EFFECTMASK_HEALONHIT | EFFECTMASK_KNOCKBACK,
            },
        [ABILITY_STATE_DASH][0] =
            {
                .state    = ABILITY_STATE_DASH,
                .cooldown = 2.0f,
                .duration = 0.2f,
            },
        [ABILITY_STATE_DASH][1] =
            {
                .state    = ABILITY_STATE_DASH,
                .cooldown = 1.5f,
                .duration = 0.3f,
            },
        [ABILITY_STATE_DASH][2] =
            {
                .state      = ABILITY_STATE_DASH,
                .cooldown   = 1.0f,
                .duration   = 0.5f,
                .damage     = 10,
                .buffMask   = BITC(BUFFKIND_STUN),
                .effectMask = EFFECTMASK_KNOCKBACK | EFFECTMASK_REFLECTPROJECTILE,
            },
        [ABILITY_STATE_PISTOL][0] =
            {
                .state    = ABILITY_STATE_PISTOL,
                .cooldown = 0.3f,
                .damage   = 5,
            },
        [ABILITY_STATE_PISTOL][1] =
            {
                .state    = ABILITY_STATE_PISTOL,
                .cooldown = 0.2f,
                .damage   = 8,
            },
        [ABILITY_STATE_PISTOL][2] =
            {
                .state      = ABILITY_STATE_PISTOL,
                .cooldown   = 0.1f,
                .damage     = 8,
                .buffMask   = BITC(BUFFKIND_FIRE),
                .effectMask = EFFECTMASK_CHAINLIGHTNING,
            },
        [ABILITY_STATE_CLAW][0] =
            {
                .state    = ABILITY_STATE_CLAW,
                .cooldown = 1.5f,
                .duration = 0.5f,
                .damage   = 25,
            },
        [ABILITY_STATE_CLAW][1] =
            {
                .state    = ABILITY_STATE_CLAW,
                .cooldown = 1.2f,
                .duration = 0.5f,

                .damage = 30,
            },
        [ABILITY_STATE_CLAW][2] =
            {
                .state      = ABILITY_STATE_CLAW,
                .cooldown   = 0.8f,
                .duration   = 0.5f,
                .effectMask = EFFECTMASK_KNOCKBACK | EFFECTMASK_REFLECTPROJECTILE | EFFECTMASK_CHAINLIGHTNING |
                              EFFECTMASK_HEALONHIT,
                .damage     = 35,
            },
        [ABILITY_STATE_LASER][0] =
            {
                .state    = ABILITY_STATE_LASER,
                .cooldown = 2.0f,
                .damage   = 20,
            },
        [ABILITY_STATE_LASER][1] =
            {
                .state    = ABILITY_STATE_LASER,
                .cooldown = 2.0f,
                .damage   = 25,
            },
        [ABILITY_STATE_LASER][2] =
            {
                .state      = ABILITY_STATE_LASER,
                .cooldown   = 2.0f,
                .damage     = 30,
                .effectMask = EFFECTMASK_KNOCKBACK | EFFECTMASK_CHAINLIGHTNING,
            },

        [ABILITY_STATE_FIREBALLVOLLEY][0] =
            {
                .state    = ABILITY_STATE_FIREBALLVOLLEY,
                .cooldown = 6.0f,
            },
};