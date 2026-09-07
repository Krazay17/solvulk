#include "s_ability.h"

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
            .cooldown   = 1.0f,
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
            .duration   = 0.6f,
            .cooldown   = 0.1f,
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

const AbilityStateFunc ABILITY_STATE_FUNC[ABILITY_STATE_COUNT] = {
    [ABILITY_STATE_IDLE] =
        {
            Ability_Idle_Update,
            Ability_Idle_Enter,
            Ability_Idle_Exit,
            Ability_Idle_CanExit,
            Ability_Idle_CanEnter,
        },
    [ABILITY_STATE_DASH] =
        {
            Ability_Dash_Update,
            Ability_Dash_Enter,
            Ability_Dash_Exit,
            Ability_Dash_CanExit,
            Ability_Dash_CanEnter,
        },
    [ABILITY_STATE_CLAW] =
        {
            Ability_Claw_Update,
            Ability_Claw_Enter,
            Ability_Claw_Exit,
            Ability_Claw_CanExit,
            Ability_Claw_CanEnter,
            Ability_Claw_Draw,
        },
    // [ABILITY_STATE_FIREBALL] =
    //     {
    //         Fireball_State_Update,
    //         Fireball_State_Enter,
    //         Fireball_State_Exit,
    //         Fireball_State_CanExit,
    //         Fireball_State_CanEnter,
    //         Fireball_State_Draw,
    //     },
    // [ABILITY_STATE_SHIELD] =
    //     {
    //         Shield_State_Update,
    //         Shield_State_Enter,
    //         Shield_State_Exit,
    //         Shield_State_CanExit,
    //         Shield_State_CanEnter,
    //         Shield_State_Draw,
    //     },
    // [ABILITY_STATE_PISTOL] =
    //     {
    //         Pistol_State_Update,
    //         Pistol_State_Enter,
    //         Pistol_State_Exit,
    //         Pistol_State_CanExit,
    //         Pistol_State_CanEnter,
    //     },
    // [ABILITY_STATE_SPINSLASH] =
    //     {
    //         Spinslash_State_Update,
    //         Spinslash_State_Enter,
    //         Spinslash_State_Exit,
    //         Spinslash_State_CanExit,
    //         Spinslash_State_CanEnter,
    //     },
    // [ABILITY_STATE_LASER] =
    //     {
    //         Laser_State_Update,
    //         Laser_State_Enter,
    //         Laser_State_Exit,
    //         Laser_State_CanExit,
    //         Laser_State_CanEnter,
    //         Laser_State_Draw,
    //     },
    // [ABILITY_STATE_WHIP] =
    //     {
    //         Whip_State_Update,
    //         Whip_State_Enter,
    //         Whip_State_Exit,
    //         Whip_State_CanExit,
    //         Whip_State_CanEnter,
    //     },
    // [ABILITY_STATE_FIREBALLVOLLEY] = SCRIPT_STATE_FUNCS,
};